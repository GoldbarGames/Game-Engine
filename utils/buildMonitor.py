"""
Build Monitor - see when every game (and the engine) was last built, and
rebuild any of them from one place.

For each project folder in the solution root that has a .vcxproj, shows the
newest built exe found in any known output location (solution x64/Win32 x
Debug/Release outputs, plus the project-local x64\\<cfg> folder older
vcxproj-direct builds used), with its config/platform and timestamp.

Rows turn RED when a game's newest build is older than the engine built for
the same platform/config - running such an exe against the newer
GameEngine.dll risks the ABI-mismatch startup crash, so it must be rebuilt
first (the same condition the Play*.bat launchers warn about).

Each row also has a Play button that launches the newest build the way the
Play*.bat launchers do (cwd = the game folder for relative assets, PATH
prefixed with the matching engine output folder for the DLLs, own console
window). Play is only clickable when a build exists AND it isn't out of
date. The shared "Play args" field is passed to whichever game is launched
(e.g. --windowed --editscene russet_volcano).

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
import threading
import subprocess
import datetime
import tkinter as tk
from tkinter import ttk

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


def describe_newest(builds):
    """'2026-08-16 01:49  Debug | x64' for the newest build, or 'never built'."""
    if not builds:
        return "never built"
    mtime, plat, cfg, _path = builds[0]
    stamp = datetime.datetime.fromtimestamp(mtime).strftime("%Y-%m-%d %H:%M")
    return "%s   %s | %s" % (stamp, cfg, plat)


def engine_build_times():
    """Newest engine artifact per (platform, config): {("x64","Debug"): mtime, ...}"""
    times = {}
    for mtime, plat, cfg, _path in find_builds(ENGINE_PROJECT):
        key = (plat, cfg)
        if key not in times or mtime > times[key]:
            times[key] = mtime
    return times


def is_stale(builds, engine_times):
    """True if the game's newest build is OLDER than the engine built for the
    same platform/config - running it risks the ABI-mismatch startup crash,
    so it needs a rebuild first. Never-built games aren't 'stale'."""
    if not builds:
        return False
    mtime, plat, cfg, _path = builds[0]
    engine_mtime = engine_times.get((plat, cfg))
    return engine_mtime is not None and engine_mtime > mtime


def build_play_command(name, builds, extra_args=""):
    """(cmd_list, cwd, env) to launch a game's newest build, mirroring the
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
        import shlex
        args = shlex.split(extra_args)
    except ValueError:
        args = extra_args.split()

    return [exe_path] + args, os.path.join(SOLUTION_ROOT, name), env


# ================================== GUI ======================================

class BuildMonitor:
    def __init__(self, root):
        self.root = root
        self.root.title("Build Monitor")
        self.root.geometry("1000x640")

        self.msbuild = find_msbuild()
        self.build_buttons = []      # disabled while a build runs
        self.play_buttons = []       # also disabled while a build runs (file locks)
        self.building = False
        self.log_queue = queue.Queue()

        self.create_ui()
        self.refresh()
        self.root.after(100, self.drain_log_queue)

    # ------------------------------------------------------------------ UI
    def create_ui(self):
        top = ttk.Frame(self.root)
        top.pack(fill=tk.X, padx=10, pady=(10, 4))

        ttk.Label(top, text="Build Monitor", font=("Arial", 15, "bold")).pack(side=tk.LEFT)

        self.refresh_btn = ttk.Button(top, text="Refresh", command=self.refresh)
        self.refresh_btn.pack(side=tk.RIGHT, padx=4)

        ttk.Label(top, text="Platform:").pack(side=tk.LEFT, padx=(25, 2))
        self.platform_var = tk.StringVar(value="x64")
        ttk.Combobox(top, textvariable=self.platform_var, values=["x64", "Win32"],
                     width=7, state="readonly").pack(side=tk.LEFT)

        ttk.Label(top, text="Config:").pack(side=tk.LEFT, padx=(12, 2))
        self.config_var = tk.StringVar(value="Debug")
        ttk.Combobox(top, textvariable=self.config_var, values=["Debug", "Release"],
                     width=8, state="readonly").pack(side=tk.LEFT)

        # One shared argument line, used by every Play button
        # (e.g. --windowed --editscene russet_volcano).
        ttk.Label(top, text="Play args:").pack(side=tk.LEFT, padx=(16, 2))
        self.play_args_var = tk.StringVar(value="")
        # fill=X + expand: the entry stretches across all remaining top-bar
        # width, right up to the Refresh button (packed RIGHT earlier, so it
        # already owns the right edge).
        ttk.Entry(top, textvariable=self.play_args_var).pack(
            side=tk.LEFT, fill=tk.X, expand=True, padx=(0, 8))

        # --- engine row (pinned above the game list) -------------------------
        engine_frame = ttk.LabelFrame(self.root, text="Engine", padding=6)
        engine_frame.pack(fill=tk.X, padx=10, pady=4)
        ttk.Label(engine_frame, text=ENGINE_PROJECT, width=24,
                  font=("Arial", 10, "bold")).pack(side=tk.LEFT)
        self.engine_info = ttk.Label(engine_frame, text="", width=34)
        self.engine_info.pack(side=tk.LEFT)
        btn = ttk.Button(engine_frame, text="Rebuild Engine",
                         command=lambda: self.start_build(ENGINE_PROJECT, full_rebuild=False))
        btn.pack(side=tk.RIGHT, padx=4)
        self.build_buttons.append(btn)

        # --- scrollable game list -------------------------------------------
        list_frame = ttk.LabelFrame(self.root, text="Games", padding=2)
        list_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=4)

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
        self.canvas.bind_all("<MouseWheel>",
            lambda e: self.canvas.yview_scroll(int(-e.delta / 120), "units"))

        # --- build log -------------------------------------------------------
        log_frame = ttk.LabelFrame(self.root, text="Build log", padding=2)
        log_frame.pack(fill=tk.BOTH, padx=10, pady=(4, 2))
        # Terminal-style: dark background, light text (incl. the text cursor
        # and selection colors, which don't inherit the fg/bg).
        self.log = tk.Text(log_frame, height=9, state=tk.DISABLED, wrap=tk.NONE,
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

        self.status = ttk.Label(self.root, text="", foreground="gray")
        self.status.pack(fill=tk.X, padx=12, pady=(0, 8))

        if self.msbuild is None:
            self.set_status("ERROR: MSBuild.exe not found - builds disabled.")

    # ------------------------------------------------------------- refresh
    def refresh(self):
        """Re-scan folders + sln (picks up newly generated projects) and
        redraw every row's last-built info."""
        for child in self.rows_frame.winfo_children():
            child.destroy()
        # keep only the engine button (index 0); game buttons get recreated
        self.build_buttons = self.build_buttons[:1]
        self.play_buttons = []

        self.engine_info.config(text=describe_newest(find_builds(ENGINE_PROJECT)))

        sln_names = sln_project_names()
        folders = scan_project_folders()
        engine_times = engine_build_times()

        # Newest-built first; never-built (alphabetical) at the bottom.
        infos = []
        for name in folders:
            builds = find_builds(name)
            key = -builds[0][0] if builds else float("inf")
            infos.append((key, name, builds))
        infos.sort(key=lambda t: (t[0], t[1].lower()))

        for i, (_key, name, builds) in enumerate(infos):
            row = ttk.Frame(self.rows_frame, padding=(4, 2))
            row.pack(fill=tk.X)
            if i % 2 == 0:
                pass  # ttk frames don't restyle easily; keep it simple

            stale = is_stale(builds, engine_times)
            info = describe_newest(builds)
            color = "gray"
            if builds:
                color = "black"
            if stale:
                info += "   OUT OF DATE - engine is newer, rebuild before playing"
                color = "#c00000"

            ttk.Label(row, text=name, width=24,
                      foreground=("#c00000" if stale else "black")).pack(side=tk.LEFT)
            ttk.Label(row, text=info, width=76, foreground=color).pack(side=tk.LEFT)

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

            # Play the newest build - only clickable when a build exists AND
            # it isn't out of date vs the engine (the exact ABI-crash risk the
            # red text warns about). Disabled during builds (exe file locks).
            play = ttk.Button(row, text="Play",
                              command=lambda n=name, b=builds: self.play_game(n, b))
            play.pack(side=tk.RIGHT, padx=2)
            if not builds or stale or self.building:
                play.config(state=tk.DISABLED)
            self.play_buttons.append(play)

        if not self.building:
            self.set_status("%d project folders found (%d buildable via the solution)."
                            % (len(folders), sum(1 for _k, n, _b in infos if n in sln_names)))

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

    # ---------------------------------------------------------------- play
    def play_game(self, name, builds):
        """Launch the game's newest build in its own console window, like the
        Play*.bat launchers: cwd = the game folder, PATH += the matching
        engine output folder, plus whatever is in the shared args field."""
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


def main():
    root = tk.Tk()
    BuildMonitor(root)
    root.mainloop()


if __name__ == "__main__":
    main()
