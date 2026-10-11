"""
Build Monitor - see when every game (and the engine) was last built, and
rebuild any of them from one place.

For each project folder in the solution root that has a .vcxproj, shows the
newest exe built for the platform/config chosen in the dropdowns, found in
any known output location (the solution's x64/Win32 x Debug/Release outputs,
plus the project-local x64\\<cfg> folder older vcxproj-direct builds used),
with its timestamp. A game with no build for that platform/config says so,
and names its newest build of any kind.

Rows turn RED when a game's build is older than the engine built for the
same platform/config - running such an exe against the newer GameEngine.dll
risks the ABI-mismatch startup crash, so it must be rebuilt first (the same
condition the Play*.bat launchers warn about).

Each row also has a Play button that launches that build the way the
Play*.bat launchers do (cwd = the game folder for relative assets, PATH
prefixed with the matching engine output folder for the DLLs, own console
window). Play is only clickable when a build exists AND it isn't out of
date. The shared "Play args" field is passed to whichever game is launched
(e.g. --windowed --editscene russet_volcano).

Below the game list, two tabs share the space: the Build log, and Play args.
A row's Args button shows that game's command-line arguments in the Play
args tab (read from its source with the comments that explain each one -
gameArgs.py; games parse their own arguments, so each list is different):
the bare names in columns - click one to add it to the Play args field,
hover for its description. The tab's Descriptions button opens the full
list with every description. Starting a build switches to the Build log.

- Rebuild (game):   MSBuild <sln> -t:<Name>:Rebuild  (full rebuild, same as
                    rebuild_game.bat - a full rebuild keeps the exe newer
                    than the engine DLL so the ABI guard stays quiet)
- Rebuild Engine:   MSBuild <sln> -t:GameEngine      (same as rebuild_engine.bat)
- Refresh:          re-scans the solution folder, so newly generated projects
                    show up without restarting.

All builds go through GameEngine.sln (NOT the .vcxproj directly - a direct
build fails on the glm/glew include paths) with the CPU-safe flags
-m:1 -p:CL_MPCount=1 (one compiler process). Projects whose folder has a
.vcxproj but which are NOT registered in GameEngine.sln can't be built with
-t:<Name> (MSBuild errors on every other project) - those rows say so
instead of offering a broken button.
"""

import os
import re
import queue
import shlex
import threading
import subprocess
import datetime
import tkinter as tk
from tkinter import ttk
from tkinter import font as tkfont

import gameArgs
import slnRegistry

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
ENGINE_DIR = os.path.dirname(SCRIPT_DIR)              # .../GameEngine
SOLUTION_ROOT = os.path.dirname(ENGINE_DIR)           # .../ (parent)
SLN_PATH = os.path.join(SOLUTION_ROOT, "GameEngine.sln")

ENGINE_PROJECT = "GameEngine"

# Windows: don't pop a console window for each MSBuild run
CREATE_NO_WINDOW = 0x08000000 if os.name == "nt" else 0


# ============================ discovery (no GUI) =============================

def find_msbuild():
    """Known VS2022 path first, else vswhere - mirrors the .bat scripts."""
    known = r"C:\Program Files\Microsoft Visual Studio\2022\Community\MSBuild\Current\Bin\MSBuild.exe"
    if os.path.isfile(known):
        return known
    vswhere = os.path.join(os.environ.get("ProgramFiles(x86)", r"C:\Program Files (x86)"),
                           "Microsoft Visual Studio", "Installer", "vswhere.exe")
    if os.path.isfile(vswhere):
        try:
            out = subprocess.check_output(
                [vswhere, "-latest", "-requires", "Microsoft.Component.MSBuild",
                 "-find", r"MSBuild\**\Bin\MSBuild.exe"],
                text=True, creationflags=CREATE_NO_WINDOW)
            for line in out.splitlines():
                if line.strip() and os.path.isfile(line.strip()):
                    return line.strip()
        except Exception:
            pass
    return None


def sln_project_names():
    """Project names registered in GameEngine.sln (the only valid -t: targets)."""
    names = set()
    try:
        with open(SLN_PATH, "r", encoding="utf-8", errors="replace") as f:
            for line in f:
                m = re.match(r'\s*Project\("\{[^}]+\}"\)\s*=\s*"([^"]+)"', line)
                if m:
                    names.add(m.group(1))
    except OSError:
        pass
    return names


def scan_project_folders():
    """Top-level folders (except the engine's) containing a .vcxproj."""
    projects = []
    try:
        entries = sorted(os.listdir(SOLUTION_ROOT), key=str.lower)
    except OSError:
        return projects
    for name in entries:
        folder = os.path.join(SOLUTION_ROOT, name)
        if name == ENGINE_PROJECT or not os.path.isdir(folder):
            continue
        try:
            if any(f.lower().endswith(".vcxproj") for f in os.listdir(folder)):
                projects.append(name)
        except OSError:
            continue
    return projects


def find_builds(project, folder_name=None):
    """All built artifacts for a project: list of (mtime, platform, config, path).
    Checks the shared solution outputs and the project-local output folders."""
    folder_name = folder_name or project
    spots = [
        (os.path.join(SOLUTION_ROOT, "x64", "Debug"), "x64", "Debug"),
        (os.path.join(SOLUTION_ROOT, "x64", "Release"), "x64", "Release"),
        (os.path.join(SOLUTION_ROOT, "Debug"), "Win32", "Debug"),
        (os.path.join(SOLUTION_ROOT, "Release"), "Win32", "Release"),
        (os.path.join(SOLUTION_ROOT, folder_name, "x64", "Debug"), "x64", "Debug"),
        (os.path.join(SOLUTION_ROOT, folder_name, "x64", "Release"), "x64", "Release"),
        (os.path.join(SOLUTION_ROOT, folder_name, "Debug"), "Win32", "Debug"),
        (os.path.join(SOLUTION_ROOT, folder_name, "Release"), "Win32", "Release"),
    ]
    # The engine's main artifact is the DLL (games link it); it also has exe configs.
    filenames = [project + ".exe"]
    if project == ENGINE_PROJECT:
        filenames = [project + ".dll", project + ".exe"]

    builds = []
    for directory, plat, cfg in spots:
        for fn in filenames:
            path = os.path.join(directory, fn)
            try:
                mtime = os.path.getmtime(path)
            except OSError:
                continue
            builds.append((mtime, plat, cfg, path))
    builds.sort(reverse=True)
    return builds


def builds_for(builds, platform, config):
    """The builds made for one platform/config, newest first."""
    return [b for b in builds if b[1] == platform and b[2] == config]


def stamp(mtime):
    return datetime.datetime.fromtimestamp(mtime).strftime("%Y-%m-%d %H:%M")


def describe_build(selected, all_builds, platform, config):
    """'2026-08-16 01:49   Debug | x64' for the newest build of the chosen
    platform/config; otherwise says it has none, naming the newest build of
    any other kind, or 'never built'."""
    if selected:
        mtime, plat, cfg, _path = selected[0]
        return "%s   %s | %s" % (stamp(mtime), cfg, plat)
    if all_builds:
        mtime, plat, cfg, _path = all_builds[0]
        return "not built for %s | %s   (newest: %s | %s, %s)" % (config, platform, cfg, plat, stamp(mtime))
    return "never built"


def engine_build_times():
    """Newest engine artifact per (platform, config): {("x64","Debug"): mtime, ...}"""
    times = {}
    for mtime, plat, cfg, _path in find_builds(ENGINE_PROJECT):
        key = (plat, cfg)
        if key not in times or mtime > times[key]:
            times[key] = mtime
    return times


def is_stale(builds, engine_times):
    """True if the newest of these builds is OLDER than the engine built for
    the same platform/config - running it risks the ABI-mismatch startup
    crash, so it needs a rebuild first. Never-built games aren't 'stale'."""
    if not builds:
        return False
    mtime, plat, cfg, _path = builds[0]
    engine_mtime = engine_times.get((plat, cfg))
    return engine_mtime is not None and engine_mtime > mtime


def build_play_command(name, builds, extra_args=""):
    """(cmd_list, cwd, env) to launch the newest of these builds, mirroring the
    Play*.bat launchers: working dir = the game's project folder (relative
    asset paths), PATH prefixed with the engine's output folder for the SAME
    platform/config (GameEngine.dll, SDL2, glew, ...)."""
    _mtime, plat, cfg, exe_path = builds[0]

    if plat == "x64":
        dll_dir = os.path.join(SOLUTION_ROOT, "x64", cfg)
    else:
        dll_dir = os.path.join(SOLUTION_ROOT, cfg)

    env = dict(os.environ)
    env["PATH"] = dll_dir + os.pathsep + env.get("PATH", "")

    try:
        args = shlex.split(extra_args)
    except ValueError:
        args = extra_args.split()

    return [exe_path] + args, os.path.join(SOLUTION_ROOT, name), env


# ================================== GUI ======================================

class BuildMonitor:
    def __init__(self, root):
        self.root = root
        self.root.title("Build Monitor")
        self.root.geometry("1100x800")

        self.msbuild = find_msbuild()
        self.build_buttons = []      # disabled while a build runs
        self.play_buttons = []       # also disabled while a build runs (file locks)
        self.building = False
        self.log_queue = queue.Queue()
        self.arg_counts = {}         # game -> how many arguments its source reads
        self.args_game = None        # the game in the Play args tab
        self.args_windows = {}       # game -> its open descriptions window

        self.create_ui()
        self.refresh(rescan_args=True)
        self.root.after(100, self.drain_log_queue)

    # ------------------------------------------------------------------ UI
    def create_ui(self):
        top = ttk.Frame(self.root)
        top.pack(fill=tk.X, padx=10, pady=(10, 4))

        ttk.Label(top, text="Build Monitor", font=("Arial", 15, "bold")).pack(side=tk.LEFT)

        self.refresh_btn = ttk.Button(top, text="Refresh",
                                      command=lambda: self.refresh(rescan_args=True))
        self.refresh_btn.pack(side=tk.RIGHT, padx=4)
        # Empties the Play args field (its entry stretches up to here).
        ttk.Button(top, text="Clear", width=6,
                   command=lambda: self.play_args_var.set("")).pack(side=tk.RIGHT, padx=(0, 20))

        # The rows show the builds for this platform/config (and Rebuild and
        # Play use it), so changing either redraws them.
        ttk.Label(top, text="Platform:").pack(side=tk.LEFT, padx=(25, 2))
        self.platform_var = tk.StringVar(value="x64")
        platform_box = ttk.Combobox(top, textvariable=self.platform_var, values=["x64", "Win32"],
                                    width=7, state="readonly")
        platform_box.pack(side=tk.LEFT)
        platform_box.bind("<<ComboboxSelected>>", lambda e: self.refresh())

        ttk.Label(top, text="Config:").pack(side=tk.LEFT, padx=(12, 2))
        self.config_var = tk.StringVar(value="Debug")
        config_box = ttk.Combobox(top, textvariable=self.config_var, values=["Debug", "Release"],
                                  width=8, state="readonly")
        config_box.pack(side=tk.LEFT)
        config_box.bind("<<ComboboxSelected>>", lambda e: self.refresh())

        # One shared argument line, used by every Play button
        # (e.g. --windowed --editscene russet_volcano).
        ttk.Label(top, text="Play args:").pack(side=tk.LEFT, padx=(16, 2))
        self.play_args_var = tk.StringVar(value="")
        # fill=X + expand: the entry stretches across all remaining top-bar
        # width, right up to the Clear button (packed RIGHT earlier, so it
        # and Refresh already own the right edge).
        self.play_args_entry = ttk.Entry(top, textvariable=self.play_args_var)
        self.play_args_entry.pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(0, 4))

        # --- engine row (pinned above the game list) -------------------------
        engine_frame = ttk.LabelFrame(self.root, text="Engine", padding=6)
        engine_frame.pack(fill=tk.X, padx=10, pady=4)
        ttk.Label(engine_frame, text=ENGINE_PROJECT, width=24,
                  font=("Arial", 10, "bold")).pack(side=tk.LEFT)
        self.engine_info = ttk.Label(engine_frame, text="", width=76)
        self.engine_info.pack(side=tk.LEFT)
        btn = ttk.Button(engine_frame, text="Rebuild Engine",
                         command=lambda: self.start_build(ENGINE_PROJECT, full_rebuild=False))
        btn.pack(side=tk.RIGHT, padx=4)
        self.build_buttons.append(btn)

        self.status = ttk.Label(self.root, text="", foreground="gray")
        self.status.pack(side=tk.BOTTOM, fill=tk.X, padx=12, pady=(0, 8))

        # The game list above the tabs (build log / play args); drag the
        # line between them to share the height differently.
        panes = ttk.PanedWindow(self.root, orient=tk.VERTICAL)
        panes.pack(fill=tk.BOTH, expand=True, padx=10, pady=4)

        # --- scrollable game list -------------------------------------------
        list_frame = ttk.LabelFrame(panes, text="Games", padding=2)

        self.canvas = tk.Canvas(list_frame, highlightthickness=0)
        scrollbar = ttk.Scrollbar(list_frame, orient=tk.VERTICAL, command=self.canvas.yview)
        self.rows_frame = ttk.Frame(self.canvas)
        self.rows_frame.bind("<Configure>",
            lambda e: self.canvas.configure(scrollregion=self.canvas.bbox("all")))
        self.canvas_window = self.canvas.create_window((0, 0), window=self.rows_frame, anchor="nw")
        self.canvas.bind("<Configure>",
            lambda e: self.canvas.itemconfigure(self.canvas_window, width=e.width))
        self.canvas.configure(yscrollcommand=scrollbar.set)
        self.canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)
        self.canvas.bind_all("<MouseWheel>", self.on_mouse_wheel)

        self.tabs = ttk.Notebook(panes)
        panes.add(list_frame, weight=1)
        panes.add(self.tabs, weight=0)

        # --- build log tab ---------------------------------------------------
        log_frame = ttk.Frame(self.tabs, padding=2)
        self.tabs.add(log_frame, text="Build log")
        self.log_tab = log_frame
        # Terminal-style: dark background, light text (incl. the text cursor
        # and selection colors, which don't inherit the fg/bg).
        self.log = tk.Text(log_frame, height=12, state=tk.DISABLED, wrap=tk.NONE,
                           font=("Consolas", 9),
                           background="#1e1e1e", foreground="#d4d4d4",
                           insertbackground="#d4d4d4",
                           selectbackground="#264f78", selectforeground="#ffffff")
        # Line-color tags for MSBuild output (matched in append_log).
        self.log.tag_configure("error", foreground="#f48771")
        self.log.tag_configure("warning", foreground="#dcdcaa")
        self.log.tag_configure("success", foreground="#89d185")
        log_scroll = ttk.Scrollbar(log_frame, orient=tk.VERTICAL, command=self.log.yview)
        self.log.configure(yscrollcommand=log_scroll.set)
        self.log.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        log_scroll.pack(side=tk.RIGHT, fill=tk.Y)

        # --- play args tab ---------------------------------------------------
        # One game's arguments as bare names: click one to add it to the Play
        # args field, hover for what it does. A row's Args button (or the
        # Game dropdown here) picks the game; Descriptions opens the full
        # list with every comment.
        self.args_tab = ttk.Frame(self.tabs, padding=(6, 6, 6, 2))
        self.tabs.add(self.args_tab, text="Play args")
        bar = ttk.Frame(self.args_tab)
        bar.pack(fill=tk.X, pady=(0, 4))
        ttk.Label(bar, text="Game:").pack(side=tk.LEFT, padx=(0, 4))
        self.args_game_var = tk.StringVar(value="")
        self.args_game_box = ttk.Combobox(bar, textvariable=self.args_game_var,
                                          width=24, state="readonly")
        self.args_game_box.pack(side=tk.LEFT)
        self.args_game_box.bind("<<ComboboxSelected>>",
                                lambda e: self.show_args(self.args_game_var.get()))
        self.args_summary = ttk.Label(bar, text="", foreground="gray")
        self.args_summary.pack(side=tk.LEFT, padx=12)
        self.descriptions_btn = ttk.Button(bar, text="Descriptions...", state=tk.DISABLED,
                                           command=lambda: self.show_descriptions(self.args_game))
        self.descriptions_btn.pack(side=tk.RIGHT)
        self.args_filter_var = tk.StringVar()
        ttk.Entry(bar, textvariable=self.args_filter_var, width=24).pack(side=tk.RIGHT, padx=(0, 12))
        ttk.Label(bar, text="Filter:").pack(side=tk.RIGHT, padx=(0, 4))
        self.args_filter_var.trace_add(
            "write", lambda *_: self.picker.set_filter(self.args_filter_var.get()))

        self.picker = ArgPicker(self.args_tab, self.play_args_var, self.on_arg_added)
        self.picker.frame.pack(fill=tk.BOTH, expand=True)

        if self.msbuild is None:
            self.set_status("ERROR: MSBuild.exe not found - builds disabled.")

    def on_mouse_wheel(self, event):
        """Scroll the game list - only when the pointer is over it, so the
        wheel over the build log or an Args window doesn't move it too."""
        if str(event.widget).startswith(str(self.canvas)):
            self.canvas.yview_scroll(int(-event.delta / 120), "units")

    # ------------------------------------------------------------- refresh
    def refresh(self, rescan_args=False):
        """Re-scan folders + sln (picks up newly generated projects) and
        redraw every row's build info for the chosen platform/config.
        rescan_args also re-reads every game's arguments for the Args
        buttons (about half a second, so not on every dropdown change)."""
        for child in self.rows_frame.winfo_children():
            child.destroy()
        # keep only the engine button (index 0); game buttons get recreated
        self.build_buttons = self.build_buttons[:1]
        self.play_buttons = []

        platform = self.platform_var.get()
        config = self.config_var.get()

        engine_builds = find_builds(ENGINE_PROJECT)
        self.engine_info.config(text=describe_build(builds_for(engine_builds, platform, config),
                                                    engine_builds, platform, config))

        sln_names = sln_project_names()
        folders = scan_project_folders()
        engine_times = engine_build_times()
        if rescan_args:
            self.arg_counts = {name: len(gameArgs.scan_game(os.path.join(SOLUTION_ROOT, name)))
                               for name in folders}
            if self.args_game is not None:
                self.show_args(self.args_game, select_tab=False)   # its source may have changed
        # The Play args tab's game list: every game that takes arguments (or
        # that is new since the last scan, so not known yet).
        self.args_game_box.config(values=[n for n in folders if self.arg_counts.get(n) != 0])

        # Newest-built (for this platform/config) first; the rest
        # (alphabetical) at the bottom.
        infos = []
        for name in folders:
            all_builds = find_builds(name)
            builds = builds_for(all_builds, platform, config)
            key = -builds[0][0] if builds else float("inf")
            infos.append((key, name, builds, all_builds))
        infos.sort(key=lambda t: (t[0], t[1].lower()))

        for _key, name, builds, all_builds in infos:
            row = ttk.Frame(self.rows_frame, padding=(4, 2))
            row.pack(fill=tk.X)

            stale = is_stale(builds, engine_times)
            info = describe_build(builds, all_builds, platform, config)
            color = "gray"
            if builds:
                color = "black"
            if stale:
                info += "   OUT OF DATE - engine is newer, rebuild before playing"
                color = "#c00000"

            # The buttons are packed before the labels, so a narrow window
            # clips the end of the info text rather than the buttons.
            if name in sln_names:
                btn = ttk.Button(row, text="Rebuild",
                                 command=lambda n=name: self.start_build(n, full_rebuild=True))
                btn.pack(side=tk.RIGHT, padx=4)
                if self.building or self.msbuild is None:
                    btn.config(state=tk.DISABLED)
                self.build_buttons.append(btn)
            else:
                # Not buildable until it's in the .sln - offer to register it
                # (writes the Project block + config mappings; .bak backup).
                btn = ttk.Button(row, text="Register in .sln",
                                 command=lambda n=name: self.register_project(n))
                btn.pack(side=tk.RIGHT, padx=4)
                if self.building:
                    btn.config(state=tk.DISABLED)
                self.build_buttons.append(btn)
                ttk.Label(row, text="not in GameEngine.sln",
                          foreground="#a06000").pack(side=tk.RIGHT, padx=8)

            # Play the build for the chosen platform/config - only clickable
            # when there is one AND it isn't out of date vs the engine (the
            # exact ABI-crash risk the red text warns about). Disabled during
            # builds (exe file locks).
            play = ttk.Button(row, text="Play",
                              command=lambda n=name, b=builds: self.play_game(n, b))
            play.pack(side=tk.RIGHT, padx=2)
            if not builds or stale or self.building:
                play.config(state=tk.DISABLED)
            self.play_buttons.append(play)

            # Shows the game's command-line arguments in the Play args tab. A
            # game added since the last scan (count unknown) still gets a
            # live button; the tab scans its source when it shows it.
            count = self.arg_counts.get(name)
            args_btn = ttk.Button(row, text="Args" if not count else "Args (%d)" % count, width=9,
                                  command=lambda n=name: self.show_args(n))
            args_btn.pack(side=tk.RIGHT, padx=2)
            if count == 0:
                args_btn.config(state=tk.DISABLED)

            ttk.Label(row, text=name, width=24,
                      foreground=("#c00000" if stale else "black")).pack(side=tk.LEFT)
            ttk.Label(row, text=info, width=88, anchor=tk.W, foreground=color).pack(side=tk.LEFT)

        if not self.building:
            self.set_status("%d project folders found (%d buildable via the solution)."
                            % (len(folders), sum(1 for _k, n, _b, _a in infos if n in sln_names)))

    # ------------------------------------------------------------ register
    def register_project(self, name):
        """Add the project's .vcxproj to GameEngine.sln (Project block +
        config mappings, .bak backup) so it becomes buildable via -t:."""
        folder = os.path.join(SOLUTION_ROOT, name)
        ok, message = slnRegistry.register_project(SLN_PATH, folder)
        self.append_log("register %s: %s\n" % (name, message))
        self.set_status(message)
        if ok:
            self.refresh()

    # ---------------------------------------------------------------- args
    def show_args(self, name, select_tab=True):
        """Put the game's arguments in the Play args tab, read fresh from its
        source, and (normally) switch to that tab."""
        if not name:
            return
        args = gameArgs.scan_game(os.path.join(SOLUTION_ROOT, name))
        self.args_game = name
        self.args_game_var.set(name)
        self.arg_counts[name] = len(args)
        self.picker.set_args(args, "No command-line arguments found in %s's source." % name)
        if args:
            files = sorted({a.path for a in args})
            self.args_summary.config(text="%d arguments, read from %s" % (len(args), ", ".join(files)))
        else:
            self.args_summary.config(text="")
        self.descriptions_btn.config(state=tk.NORMAL if args else tk.DISABLED)
        if select_tab:
            self.tabs.select(self.args_tab)

    def on_arg_added(self):
        """After a click adds an argument: cursor at the end of the Play args
        field, ready for the argument's value."""
        self.play_args_entry.focus_set()
        self.play_args_entry.icursor(tk.END)
        self.play_args_entry.xview_moveto(1.0)

    def show_descriptions(self, name):
        """Open the game's arguments with their full descriptions, or bring
        that window forward if it's open."""
        if not name:
            return
        window = self.args_windows.get(name)
        if window is not None and window.top.winfo_exists():
            window.top.deiconify()
            window.top.lift()
            window.top.focus_force()
            return
        self.args_windows[name] = ArgsWindow(self.root, name, os.path.join(SOLUTION_ROOT, name))

    # ---------------------------------------------------------------- play
    def play_game(self, name, builds):
        """Launch the game's build for the chosen platform/config in its own
        console window, like the Play*.bat launchers: cwd = the game folder,
        PATH += the matching engine output folder, plus whatever is in the
        shared args field."""
        if not builds:
            return
        cmd, cwd, env = build_play_command(name, builds, self.play_args_var.get())
        try:
            flags = 0x00000010 if os.name == "nt" else 0   # CREATE_NEW_CONSOLE
            subprocess.Popen(cmd, cwd=cwd, env=env, creationflags=flags)
            self.append_log("play %s: %s\n" % (name, subprocess.list2cmdline(cmd)))
            self.set_status("Playing %s" % name)
        except Exception as e:
            self.append_log("play %s FAILED: %s\n" % (name, e))
            self.set_status("Failed to launch %s" % name)

    # --------------------------------------------------------------- build
    def start_build(self, project, full_rebuild):
        if self.building or self.msbuild is None:
            return
        self.building = True
        for btn in self.build_buttons:
            btn.config(state=tk.DISABLED)
        for btn in self.play_buttons:
            btn.config(state=tk.DISABLED)
        self.refresh_btn.config(state=tk.DISABLED)
        self.tabs.select(self.log_tab)       # show the build's output

        config = self.config_var.get()
        platform = self.platform_var.get()
        # Games use :Rebuild (full) so the exe ends up newer than the engine
        # DLL (ABI guard); the engine itself builds incrementally, same as
        # rebuild_engine.bat. CPU-safe: one MSBuild node, one cl.exe.
        target = project + (":Rebuild" if full_rebuild else "")
        cmd = [self.msbuild, SLN_PATH, "-t:" + target,
               "-p:Configuration=" + config, "-p:Platform=" + platform,
               "-m:1", "-p:CL_MPCount=1", "-nologo", "-v:minimal"]

        self.append_log("\n=== %s  [%s | %s]  %s ===\n"
                        % (project, config, platform,
                           datetime.datetime.now().strftime("%H:%M:%S")))
        self.set_status("Building %s [%s | %s] ... (engine builds can take several minutes)"
                        % (project, config, platform))

        threading.Thread(target=self.run_build, args=(cmd, project), daemon=True).start()

    def run_build(self, cmd, project):
        """Worker thread: stream MSBuild output into the log queue."""
        try:
            proc = subprocess.Popen(cmd, cwd=SOLUTION_ROOT,
                                    stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
                                    text=True, creationflags=CREATE_NO_WINDOW)
            for line in proc.stdout:
                self.log_queue.put(("line", line))
            rc = proc.wait()
        except Exception as e:
            self.log_queue.put(("line", "ERROR launching MSBuild: %s\n" % e))
            rc = -1
        self.log_queue.put(("done", (project, rc)))

    def drain_log_queue(self):
        """GUI-thread pump: apply queued log lines / completion events."""
        try:
            while True:
                kind, payload = self.log_queue.get_nowait()
                if kind == "line":
                    self.append_log(payload)
                elif kind == "done":
                    project, rc = payload
                    if rc == 0:
                        self.append_log("*** %s BUILD SUCCEEDED ***\n" % project)
                        if project == ENGINE_PROJECT:
                            self.append_log("NOTE: the engine DLL is now newer than every "
                                            "game exe - rebuild games before running them "
                                            "(ABI guard).\n")
                    else:
                        self.append_log("*** %s BUILD FAILED (exit %s) ***\n" % (project, rc))
                    self.building = False
                    for btn in self.build_buttons:
                        btn.config(state=tk.NORMAL)
                    self.refresh_btn.config(state=tk.NORMAL)
                    self.refresh()
                    self.set_status("%s: %s" % (project,
                                    "build succeeded" if rc == 0 else "BUILD FAILED"))
        except queue.Empty:
            pass
        self.root.after(100, self.drain_log_queue)

    # ------------------------------------------------------------- helpers
    def append_log(self, text):
        self.log.config(state=tk.NORMAL)
        # Tint per line: errors red, warnings yellow, success green.
        for line in text.splitlines(keepends=True):
            low = line.lower()
            # success patterns first: MSBuild's "0 Error(s)" summary would
            # otherwise match the error check
            if "build succeeded" in low or "0 error" in low or "0 warning" in low:
                tag = "success"
            elif "error" in low or "failed" in low:
                tag = "error"
            elif "warning" in low:
                tag = "warning"
            else:
                tag = ()
            self.log.insert(tk.END, line, tag)
        self.log.see(tk.END)
        self.log.config(state=tk.DISABLED)

    def set_status(self, text):
        self.status.config(text=text)


def describe_arg(a):
    """(heading, [(style, text)]) for one gameArgs.Arg, as the descriptions
    window and the tooltips show it. The heading is its own usage line when
    it has one ("--shots N"); other forms ("--deducetest solve", or a line
    shared with another flag) become sub-headings. Usage lines come first,
    then the comments where the code reads it."""
    title = a.name
    for head, _text in a.usage:
        if head == a.name or head.startswith(a.name + " "):
            title = head
            break
    parts = []
    for head, text in a.usage:
        if head in (title, a.name):
            if text:
                parts.append(("text", text))
        else:
            parts.append(("head", head))
            if text:
                parts.append(("under_head", text))
    for note in a.notes:
        parts.append(("note", note))
    # Only mentioned in passing elsewhere (e.g. inside another flag's line):
    # show that line, it's all the source says.
    if not a.usage and not a.notes:
        for mention in a.mentions:
            parts.append(("dim", "mentioned: " + mention))
    if not a.described():
        parts.append(("code", "(no comment in the source)   " + a.code))
    return title, parts


def arg_search_text(a, described):
    """What a filter matches an argument against: everything shown for it."""
    title, parts = described
    return "\n".join([a.name, title] + [text for _style, text in parts]).lower()


def scroll_text(widget, event):
    widget.yview_scroll(int(-event.delta / 120), "units")
    return "break"


class ArgPicker:
    """One game's command-line arguments as bare names in columns, read top
    to bottom like an index: click one to add it to Play args, hover for what
    it does. Names already in Play args are tinted. It is the main window's
    Play args tab."""

    NAME_FONT = ("Consolas", 10)
    NAME_PADX = 8
    TIP_DELAY_MS = 350

    def __init__(self, parent, play_args_var, on_added):
        self.play_args = play_args_var
        self.on_added = on_added           # called after a click adds one
        self.args = []
        self.by_name = {}
        self.sections = {}
        self.search = {}
        self.filter = ""
        self.empty_text = "Pick a game with its Args button, or from the Game list above."
        self.shown = []                    # names that pass the filter, in order
        self.columns = 0
        self.column_width = 100
        self.tip = None
        self.tip_job = None
        self.font = tkfont.Font(font=self.NAME_FONT)

        self.frame = ttk.Frame(parent)
        self.names = tk.Text(self.frame, wrap=tk.NONE, padx=self.NAME_PADX, pady=4, height=14,
                             font=self.NAME_FONT, relief=tk.FLAT, highlightthickness=0,
                             cursor="arrow", state=tk.DISABLED)
        scroll = ttk.Scrollbar(self.frame, orient=tk.VERTICAL, command=self.names.yview)
        self.names.configure(yscrollcommand=scroll.set)
        scroll.pack(side=tk.RIGHT, fill=tk.Y)
        self.names.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

        n = self.names
        n.tag_configure("name", foreground="#1a4f8b")
        n.tag_configure("added", background="#dcefdc", foreground="#1e6b1e")   # already in Play args
        n.tag_configure("hover", background="#d6e6f7", underline=True)         # configured last: wins
        n.tag_configure("dim", foreground="#808080", font=("Segoe UI", 10))
        # A click picks a name; it doesn't start a text selection. (The
        # per-name tag bindings still run - "break" doesn't stop them.)
        for sequence in ("<Button-1>", "<B1-Motion>", "<Double-1>", "<Triple-1>", "<Shift-Button-1>"):
            n.bind(sequence, lambda e: "break")
        n.bind("<MouseWheel>", lambda e: scroll_text(self.names, e))
        n.bind("<Configure>", self.on_resize)

        # Typing in Play args re-marks the names already in it.
        self.play_args.trace_add("write", lambda *_: self.mark_added())
        self.render()

    def set_args(self, args, empty_text):
        """Show another game's arguments (or the same game's, re-read)."""
        self.hide_tip()
        self.args = args
        self.by_name = {a.name: a for a in args}
        self.sections = {a.name: describe_arg(a) for a in args}
        self.search = {a.name: arg_search_text(a, self.sections[a.name]) for a in args}
        self.empty_text = empty_text
        # Columns as wide as this game's longest name, whatever the filter
        # shows, so names don't jump about while typing in it.
        widest = max([self.font.measure(a.name) for a in args] or [0])
        self.column_width = widest + 28
        self.names.config(tabs=(self.column_width, 2 * self.column_width))
        for a in args:
            tag = self.name_tag(a.name)
            self.names.tag_bind(tag, "<Enter>", lambda e, f=a.name: self.on_name_enter(f))
            self.names.tag_bind(tag, "<Leave>", lambda e: self.on_name_leave())
            self.names.tag_bind(tag, "<Button-1>", lambda e, f=a.name: self.add_arg(f))
        self.render()

    def set_filter(self, text):
        self.filter = text.strip().lower()
        self.render()

    @staticmethod
    def name_tag(name):
        return "arg:" + name

    def columns_for(self, width):
        return max(1, (width - 2 * self.NAME_PADX) // self.column_width)

    def render(self):
        self.hide_tip()
        self.shown = [a.name for a in self.args if not self.filter or self.filter in self.search[a.name]]
        n = self.names
        n.config(state=tk.NORMAL)
        n.delete("1.0", tk.END)
        width = n.winfo_width()
        self.columns = self.columns_for(width if width > 50 else 1040)   # 1040: not drawn yet
        rows = -(-len(self.shown) // self.columns)
        for r in range(rows):
            if r:
                n.insert(tk.END, "\n")
            for c in range(self.columns):
                i = c * rows + r
                if i >= len(self.shown):
                    break
                if c:
                    n.insert(tk.END, "\t")
                n.insert(tk.END, self.shown[i], ("name", self.name_tag(self.shown[i])))
        if not self.args:
            n.insert(tk.END, self.empty_text, "dim")
        elif not self.shown:
            n.insert(tk.END, "Nothing matches \"%s\"." % self.filter, "dim")
        self.mark_added()
        n.config(state=tk.DISABLED)
        n.yview_moveto(0)

    def on_resize(self, event):
        if self.columns_for(event.width) != self.columns:
            self.render()

    # ---------------------------------------------------- click to add
    def flags_in_play_args(self):
        try:
            words = shlex.split(self.play_args.get())
        except ValueError:                    # an unclosed quote, mid-typing
            words = self.play_args.get().split()
        return {w for w in words if w.startswith("--")}

    def mark_added(self):
        """Tint the names that are already in Play args."""
        n = self.names
        n.tag_remove("added", "1.0", tk.END)
        present = self.flags_in_play_args()
        for name in self.shown:
            if name in present:
                span = n.tag_ranges(self.name_tag(name))
                if span:
                    n.tag_add("added", span[0], span[1])

    def add_arg(self, name):
        """Append the flag to Play args, followed by a space for its value.
        Adding one twice is allowed (--press and --simchoice can be given
        more than once)."""
        current = self.play_args.get().rstrip()
        self.play_args.set((current + " " if current else "") + name + " ")
        self.hide_tip()
        self.on_added()

    def on_name_enter(self, name):
        span = self.names.tag_ranges(self.name_tag(name))
        if span:
            self.names.tag_add("hover", span[0], span[1])
        self.names.config(cursor="hand2")
        self.hide_tip()
        self.tip_job = self.names.after(self.TIP_DELAY_MS, lambda: self.show_tip(name))

    def on_name_leave(self):
        self.names.tag_remove("hover", "1.0", tk.END)
        self.names.config(cursor="arrow")
        self.hide_tip()

    # ------------------------------------------------------------ tooltip
    def show_tip(self, name):
        self.tip_job = None
        if self.tip is not None or name not in self.sections:
            return
        title, parts = self.sections[name]
        a = self.by_name[name]
        tip = tk.Toplevel(self.names)
        tip.wm_overrideredirect(True)
        box = tk.Frame(tip, background="#ffffe1", borderwidth=1, relief=tk.SOLID)
        box.pack()
        heading = tk.Frame(box, background="#ffffe1")
        heading.pack(fill=tk.X, padx=8, pady=(6, 2))
        tk.Label(heading, text=title, font=("Consolas", 10, "bold"), foreground="#1a4f8b",
                 background="#ffffe1").pack(side=tk.LEFT)
        tk.Label(heading, text="   %s:%d" % (a.path, a.line), font=("Segoe UI", 8),
                 foreground="#909090", background="#ffffe1").pack(side=tk.LEFT)
        tk.Label(box, text="\n".join(text for _style, text in parts), font=("Segoe UI", 9),
                 justify=tk.LEFT, anchor=tk.W, wraplength=460,
                 background="#ffffe1").pack(fill=tk.X, padx=8, pady=(0, 6))

        # Below-right of the pointer, kept on the screen it's on.
        tip.update_idletasks()
        px, py = self.names.winfo_pointerxy()
        w, h = tip.winfo_reqwidth(), tip.winfo_reqheight()
        sw, sh = tip.winfo_screenwidth(), tip.winfo_screenheight()
        x, y = px + 16, py + 20
        if 0 <= px < sw and x + w > sw:      # clamp only on the primary screen,
            x = max(0, sw - w - 4)           # whose size is all Tk reports
        if 0 <= py < sh and y + h > sh:
            y = max(0, py - h - 8)
        tip.geometry("+%d+%d" % (x, y))
        self.tip = tip

    def hide_tip(self):
        if self.tip_job is not None:
            self.names.after_cancel(self.tip_job)
            self.tip_job = None
        if self.tip is not None:
            self.tip.destroy()
            self.tip = None


class ArgsWindow:
    """Every command-line argument one game's source reads, with the
    comments that explain each, filtered as you type (the Play args tab's
    Descriptions button). Read fresh from the source each time it opens."""

    def __init__(self, parent, name, folder):
        self.args = gameArgs.scan_game(folder)
        self.by_name = {a.name: a for a in self.args}
        self.sections = {a.name: describe_arg(a) for a in self.args}
        self.search = {a.name: arg_search_text(a, self.sections[a.name]) for a in self.args}

        self.top = tk.Toplevel(parent)
        self.top.title("%s - command-line arguments" % name)
        self.top.geometry("880x700")
        self.top.bind("<Escape>", lambda e: self.top.destroy())

        bar = ttk.Frame(self.top, padding=(10, 8, 10, 6))
        bar.pack(fill=tk.X)
        if self.args:
            files = sorted({a.path for a in self.args})
            summary = "%d arguments, read from %s" % (len(self.args), ", ".join(files))
        else:
            summary = "No command-line arguments found in %s's source." % name
        ttk.Label(bar, text=summary).pack(side=tk.LEFT)
        self.filter_var = tk.StringVar()
        entry = ttk.Entry(bar, textvariable=self.filter_var, width=30)
        entry.pack(side=tk.RIGHT)
        ttk.Label(bar, text="Filter:").pack(side=tk.RIGHT, padx=(0, 4))
        self.filter_var.trace_add("write", lambda *_: self.render())

        body = ttk.Frame(self.top, padding=(10, 0, 10, 10))
        body.pack(fill=tk.BOTH, expand=True)
        self.text = tk.Text(body, wrap=tk.WORD, padx=12, pady=6, font=("Segoe UI", 10),
                            relief=tk.FLAT, borderwidth=1, highlightthickness=0)
        scroll = ttk.Scrollbar(body, orient=tk.VERTICAL, command=self.text.yview)
        self.text.configure(yscrollcommand=scroll.set)
        scroll.pack(side=tk.RIGHT, fill=tk.Y)
        self.text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        self.text.bind("<MouseWheel>", lambda e: scroll_text(self.text, e))

        t = self.text
        t.tag_configure("flag", font=("Consolas", 11, "bold"), foreground="#1a4f8b", spacing1=12)
        t.tag_configure("where", font=("Segoe UI", 9), foreground="#909090")
        t.tag_configure("head", font=("Consolas", 10), foreground="#1a4f8b",
                        lmargin1=24, lmargin2=24, spacing1=2)
        t.tag_configure("text", lmargin1=24, lmargin2=24, spacing3=2)
        t.tag_configure("under_head", lmargin1=48, lmargin2=48, spacing3=2)
        t.tag_configure("note", lmargin1=24, lmargin2=24, spacing3=2, foreground="#444444")
        t.tag_configure("dim", lmargin1=24, lmargin2=24, spacing3=2, foreground="#808080")
        t.tag_configure("code", font=("Consolas", 9), lmargin1=24, lmargin2=24, foreground="#808080")

        entry.focus_set()
        self.render()

    def render(self):
        want = self.filter_var.get().strip().lower()
        t = self.text
        t.config(state=tk.NORMAL)
        t.delete("1.0", tk.END)
        shown = 0
        for a in self.args:
            if want and want not in self.search[a.name]:
                continue
            shown += 1
            title, parts = self.sections[a.name]
            t.insert(tk.END, title, "flag")
            t.insert(tk.END, "     %s:%d\n" % (a.path, a.line), "where")
            for style, text in parts:
                t.insert(tk.END, text + "\n", style)
        if self.args and not shown:
            t.insert(tk.END, "Nothing matches \"%s\"." % want, "dim")
        t.config(state=tk.DISABLED)
        t.yview_moveto(0)


def main():
    root = tk.Tk()
    BuildMonitor(root)
    root.mainloop()


if __name__ == "__main__":
    main()
