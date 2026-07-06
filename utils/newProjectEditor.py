"""
New Project Editor - Scaffold a new game project that uses the engine.

Two modes:
  - Create:   generate a complete, ready-to-build project skeleton from the
              curated templates in utils/templates/ (boilerplate sources, a
              .vcxproj, the required config + shader files, and the directory
              tree the engine expects).
  - Validate: audit an existing project against the manifest and report exactly
              what is missing - the cure for "did I copy everything?".

Runs as a tkinter GUI by default. For scripting/testing it also has a CLI:
  python newProjectEditor.py --cli create --name MyGame --dest C:/path/to/parent
  python newProjectEditor.py --cli validate --dir C:/path/to/MyGame
"""

import os
import sys
import json
import shutil
import uuid
import argparse

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
TEMPLATES_DIR = os.path.join(SCRIPT_DIR, "templates")
PROJECT_TEMPLATE_DIR = os.path.join(TEMPLATES_DIR, "project")
PATHS_FILE = os.path.join(TEMPLATES_DIR, "paths.json")
PATHS_OVERRIDE_FILE = os.path.join(TEMPLATES_DIR, "paths.local.json")
MANIFEST_FILE = os.path.join(TEMPLATES_DIR, "manifest.json")


# ============== Core logic (no GUI - importable / CLI testable) ==============

def load_json(path):
    with open(path, "r", encoding="utf-8") as f:
        return json.load(f)


def load_manifest():
    return load_json(MANIFEST_FILE)


def derive_engine_paths():
    """Work out engine-relative paths from where this script lives, so the
    generated project keeps building even if the repo is moved/renamed.

    Layout:  <solution_root>/GameEngine/utils/newProjectEditor.py
             <solution_root>/GameEngine/src              (engine headers)
             <solution_root>/{Debug,Release,x64/...}     (engine .lib output)
    """
    engine_dir = os.path.dirname(SCRIPT_DIR)            # .../GameEngine
    solution_root = os.path.dirname(engine_dir)         # .../ (parent)
    return {
        "ENGINE_SRC": os.path.join(engine_dir, "src"),
        "ENGINE_LIB_WIN32_DEBUG": os.path.join(solution_root, "Debug"),
        "ENGINE_LIB_WIN32_RELEASE": os.path.join(solution_root, "Release"),
        "ENGINE_LIB_X64_DEBUG": os.path.join(solution_root, "x64", "Debug"),
        "ENGINE_LIB_X64_RELEASE": os.path.join(solution_root, "x64", "Release"),
    }


def load_paths():
    """Merge order (last wins): JSON defaults -> auto-derived engine paths ->
    user override file (paths.local.json). Keys starting with '_' are dropped."""
    base = {k: v for k, v in load_json(PATHS_FILE).items() if not k.startswith("_")}
    base.update(derive_engine_paths())
    if os.path.exists(PATHS_OVERRIDE_FILE):
        override = {k: v for k, v in load_json(PATHS_OVERRIDE_FILE).items()
                    if not k.startswith("_")}
        base.update(override)
    return base


def save_paths_override(paths_dict):
    with open(PATHS_OVERRIDE_FILE, "w", encoding="utf-8") as f:
        json.dump(paths_dict, f, indent=2)


def make_guid():
    return "{" + str(uuid.uuid4()).upper() + "}"


def substitute(text, ctx):
    for key, value in ctx.items():
        text = text.replace("{{" + key + "}}", str(value))
    return text


def build_context(name, window_title, game_folder, icon, paths):
    ctx = dict(paths)
    ctx["PROJECT_NAME"] = name
    ctx["WINDOW_TITLE"] = window_title
    ctx["GAME_FOLDER"] = game_folder
    ctx["ICON"] = icon
    ctx["PROJECT_GUID"] = make_guid()
    return ctx


def generate_project(name, window_title=None, game_folder=None, icon="icon.png",
                     dest_dir=None, paths=None, log=print, force=False,
                     include_wasm=True):
    """Generate a new project under dest_dir/name. Returns the project path."""
    name = name.strip()
    if not name:
        raise ValueError("Project name is required")
    if any(c in name for c in r'\/:*?"<>| '):
        raise ValueError("Project name cannot contain spaces or \\ / : * ? \" < > |")

    window_title = (window_title or name).strip()
    game_folder = (game_folder or name).strip()
    icon = (icon or "icon.png").strip()
    if not dest_dir:
        dest_dir = os.path.dirname(os.path.dirname(SCRIPT_DIR))  # solution root

    if paths is None:
        paths = load_paths()

    manifest = load_manifest()
    ctx = build_context(name, window_title, game_folder, icon, paths)

    project_dir = os.path.join(dest_dir, name)
    if os.path.isdir(project_dir) and os.listdir(project_dir) and not force:
        raise FileExistsError(
            "Target already exists and is not empty:\n" + project_dir +
            "\n(Enable 'Overwrite' to write into it anyway.)")
    os.makedirs(project_dir, exist_ok=True)

    # 1) Rendered source / project files (placeholders substituted)
    for entry in manifest["render"]:
        tpl_path = os.path.join(PROJECT_TEMPLATE_DIR, entry["template"])
        with open(tpl_path, "r", encoding="utf-8") as f:
            content = substitute(f.read(), ctx)
        out_rel = substitute(entry["output"], ctx)
        out_path = os.path.join(project_dir, out_rel)
        parent = os.path.dirname(out_path)
        if parent:
            os.makedirs(parent, exist_ok=True)
        with open(out_path, "w", encoding="utf-8", newline="\n") as f:
            f.write(content)
        log("  + " + out_rel)

    # 1b) Optional WebAssembly build files
    if include_wasm:
        for entry in manifest.get("wasm_render", []):
            tpl_path = os.path.join(PROJECT_TEMPLATE_DIR, entry["template"])
            with open(tpl_path, "r", encoding="utf-8") as f:
                content = substitute(f.read(), ctx)
            out_rel = substitute(entry["output"], ctx)
            with open(os.path.join(project_dir, out_rel), "w",
                      encoding="utf-8", newline="\n") as f:
                f.write(content)
            log("  + " + out_rel + "  (wasm)")

    # 2) Verbatim directory trees (data/, shaders, config, gui)
    for d in manifest["copy_dirs"]:
        src = os.path.join(PROJECT_TEMPLATE_DIR, d)
        dst = os.path.join(project_dir, d)
        shutil.copytree(src, dst, dirs_exist_ok=True)
        log("  + " + d + "/  (verbatim)")

    # 3) Empty directories the engine writes into at runtime
    for d in manifest["empty_dirs"]:
        os.makedirs(os.path.join(project_dir, d), exist_ok=True)
        log("  + " + d + "/")

    log("")
    log("GUID: " + ctx["PROJECT_GUID"])
    return project_dir


def validate_project(project_dir):
    """Audit a project against the manifest. Returns list of (level, message)
    where level is 'ok', 'warn', or 'error'."""
    manifest = load_manifest()
    v = manifest["validate"]
    results = []

    for d in v.get("required_dirs", []):
        ok = os.path.isdir(os.path.join(project_dir, d))
        results.append(("ok" if ok else "error",
                        ("dir   " if ok else "MISSING dir   ") + d))

    for f in v.get("required_files", []):
        ok = os.path.isfile(os.path.join(project_dir, f))
        results.append(("ok" if ok else "error",
                        ("file  " if ok else "MISSING file  ") + f))

    if v.get("require_vcxproj"):
        vcxprojs = [n for n in os.listdir(project_dir)
                    if n.lower().endswith(".vcxproj")] if os.path.isdir(project_dir) else []
        if vcxprojs:
            results.append(("ok", "vcxproj  " + ", ".join(vcxprojs)))
        else:
            results.append(("error", "MISSING  no .vcxproj found in project root"))

    if v.get("check_shaders_dat"):
        results.extend(_check_shaders_dat(project_dir))

    for f in v.get("optional_files", []):
        ok = os.path.isfile(os.path.join(project_dir, f))
        results.append(("ok" if ok else "warn",
                        ("file  " if ok else "absent (optional)  ") + f +
                        ("" if ok else "  (WebAssembly build - only needed for web)")))

    return results


def _check_shaders_dat(project_dir):
    """Cross-check every shader referenced in data/config/shaders.dat actually
    exists in data/shaders/."""
    out = []
    sd = os.path.join(project_dir, "data", "config", "shaders.dat")
    if not os.path.isfile(sd):
        out.append(("warn", "shaders.dat not found - skipping shader cross-check"))
        return out
    shader_dir = os.path.join(project_dir, "data", "shaders")
    with open(sd, "r", encoding="utf-8") as f:
        for line in f:
            parts = line.split()
            if len(parts) < 3:
                continue
            for fname in parts[1:3]:
                ok = os.path.isfile(os.path.join(shader_dir, fname))
                out.append(("ok" if ok else "error",
                            ("shader   " if ok else "MISSING shader   ") +
                            "data/shaders/" + fname + "  (referenced in shaders.dat)"))
    return out


# ============== CLI ==============

def run_cli(argv):
    parser = argparse.ArgumentParser(prog="newProjectEditor.py --cli")
    sub = parser.add_subparsers(dest="cmd", required=True)

    c = sub.add_parser("create", help="Generate a new project")
    c.add_argument("--name", required=True)
    c.add_argument("--title", default=None)
    c.add_argument("--folder", default=None, help="Game data folder name")
    c.add_argument("--icon", default="icon.png")
    c.add_argument("--dest", default=None, help="Parent directory for the project")
    c.add_argument("--force", action="store_true")
    c.add_argument("--no-wasm", dest="wasm", action="store_false",
                   help="Skip the WebAssembly build files")
    c.set_defaults(wasm=True)

    val = sub.add_parser("validate", help="Audit an existing project")
    val.add_argument("--dir", required=True)

    args = parser.parse_args(argv)

    if args.cmd == "create":
        try:
            path = generate_project(args.name, args.title, args.folder, args.icon,
                                    args.dest, force=args.force, include_wasm=args.wasm)
        except Exception as e:
            print("ERROR: " + str(e))
            return 1
        print("\nProject created at: " + path)
        results = validate_project(path)
        errors = [m for lvl, m in results if lvl == "error"]
        print("Validation: %d checks, %d problem(s)." %
              (len(results), len(errors)))
        for m in errors:
            print("  " + m)
        return 0 if not errors else 2

    if args.cmd == "validate":
        results = validate_project(args.dir)
        for lvl, msg in results:
            tag = {"ok": "[ OK ]", "warn": "[WARN]", "error": "[FAIL]"}[lvl]
            print(tag + " " + msg)
        errors = [m for lvl, m in results if lvl == "error"]
        print("\n%d check(s), %d problem(s)." % (len(results), len(errors)))
        return 0 if not errors else 2

    return 1


# ============== GUI ==============

def main_gui():
    import tkinter as tk
    from tkinter import ttk, filedialog, messagebox

    class NewProjectApp:
        def __init__(self, root):
            self.root = root
            self.root.title("New Project")
            self.root.geometry("760x620")

            self.default_dest = os.path.dirname(os.path.dirname(SCRIPT_DIR))

            self.notebook = ttk.Notebook(root)
            self.notebook.pack(fill=tk.BOTH, expand=True, padx=8, pady=8)

            self.create_create_tab()
            self.create_validate_tab()

            status = ttk.Frame(root)
            status.pack(fill=tk.X, side=tk.BOTTOM)
            ttk.Button(status, text="Configure Dependency Paths...",
                       command=self.edit_paths).pack(side=tk.LEFT, padx=6, pady=4)
            ttk.Label(status, text="Templates: " + PROJECT_TEMPLATE_DIR,
                      foreground="gray").pack(side=tk.LEFT, padx=6)

        # ---------- Create tab ----------
        def create_create_tab(self):
            frame = ttk.Frame(self.notebook)
            self.notebook.add(frame, text="Create Project")

            form = ttk.LabelFrame(frame, text="New Project", padding=10)
            form.pack(fill=tk.X, padx=8, pady=8)

            self.name_var = tk.StringVar()
            self.title_var = tk.StringVar()
            self.folder_var = tk.StringVar()
            self.icon_var = tk.StringVar(value="icon.png")
            self.dest_var = tk.StringVar(value=self.default_dest)
            self.overwrite_var = tk.BooleanVar(value=False)
            self.open_when_done_var = tk.BooleanVar(value=True)
            self.wasm_var = tk.BooleanVar(value=True)

            def row(r, label, var, browse=False, hint=""):
                ttk.Label(form, text=label).grid(row=r, column=0, sticky=tk.W, pady=4)
                entry = ttk.Entry(form, textvariable=var, width=48)
                entry.grid(row=r, column=1, sticky=tk.W + tk.E, pady=4, padx=4)
                if browse:
                    ttk.Button(form, text="Browse",
                               command=lambda: self._browse_dir(var)).grid(row=r, column=2, padx=2)
                if hint:
                    ttk.Label(form, text=hint, foreground="gray").grid(
                        row=r, column=3, sticky=tk.W, padx=4)
                return entry

            form.columnconfigure(1, weight=1)
            row(0, "Project Name", self.name_var, hint="No spaces (used for class/.vcxproj)")
            row(1, "Window Title", self.title_var, hint="Defaults to project name")
            row(2, "Game Folder", self.folder_var, hint="Game data folder; defaults to name")
            row(3, "Icon File", self.icon_var, hint="e.g. icon.png")
            row(4, "Destination", self.dest_var, browse=True, hint="Parent dir for the project")

            opts = ttk.Frame(form)
            opts.grid(row=5, column=1, columnspan=3, sticky=tk.W, pady=6)
            ttk.Checkbutton(opts, text="WebAssembly build files",
                            variable=self.wasm_var).pack(side=tk.LEFT, padx=4)
            ttk.Checkbutton(opts, text="Overwrite if exists",
                            variable=self.overwrite_var).pack(side=tk.LEFT, padx=4)
            ttk.Checkbutton(opts, text="Open folder when done",
                            variable=self.open_when_done_var).pack(side=tk.LEFT, padx=4)

            ttk.Button(form, text="Generate Project",
                       command=self.do_generate).grid(row=6, column=1, sticky=tk.W, pady=8)

            log_frame = ttk.LabelFrame(frame, text="Output", padding=6)
            log_frame.pack(fill=tk.BOTH, expand=True, padx=8, pady=4)
            self.create_log = tk.Text(log_frame, height=16, font=("Consolas", 9), wrap=tk.NONE)
            cscroll = ttk.Scrollbar(log_frame, orient=tk.VERTICAL, command=self.create_log.yview)
            self.create_log.configure(yscrollcommand=cscroll.set)
            self.create_log.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
            cscroll.pack(side=tk.RIGHT, fill=tk.Y)
            self.create_log.tag_config("err", foreground="#b00020")
            self.create_log.tag_config("ok", foreground="#0a7d28")

        def _browse_dir(self, var):
            d = filedialog.askdirectory(title="Select directory",
                                        initialdir=var.get() or self.default_dest)
            if d:
                var.set(os.path.normpath(d))

        def _clog(self, msg, tag=None):
            self.create_log.insert(tk.END, msg + "\n", tag or ())
            self.create_log.see(tk.END)
            self.root.update_idletasks()

        def do_generate(self):
            self.create_log.delete("1.0", tk.END)
            name = self.name_var.get().strip()
            try:
                self._clog("Generating '%s'..." % name)
                project_dir = generate_project(
                    name=name,
                    window_title=self.title_var.get(),
                    game_folder=self.folder_var.get(),
                    icon=self.icon_var.get(),
                    dest_dir=self.dest_var.get().strip() or None,
                    log=lambda m: self._clog(m),
                    force=self.overwrite_var.get(),
                    include_wasm=self.wasm_var.get(),
                )
            except Exception as e:
                self._clog("ERROR: " + str(e), "err")
                messagebox.showerror("Generation failed", str(e))
                return

            self._clog("")
            self._clog("Validating generated project...")
            results = validate_project(project_dir)
            errors = [m for lvl, m in results if lvl == "error"]
            if errors:
                for m in errors:
                    self._clog("  " + m, "err")
                self._clog("Done with %d problem(s)." % len(errors), "err")
            else:
                self._clog("All %d checks passed. Project is complete." % len(results), "ok")

            self._clog("")
            self._clog("Created at: " + project_dir, "ok")
            if self.open_when_done_var.get():
                self._open_folder(project_dir)

        def _open_folder(self, path):
            try:
                if sys.platform.startswith("win"):
                    os.startfile(path)  # noqa
                elif sys.platform == "darwin":
                    import subprocess
                    subprocess.Popen(["open", path])
                else:
                    import subprocess
                    subprocess.Popen(["xdg-open", path])
            except Exception:
                pass

        # ---------- Validate tab ----------
        def create_validate_tab(self):
            frame = ttk.Frame(self.notebook)
            self.notebook.add(frame, text="Validate Project")

            top = ttk.Frame(frame)
            top.pack(fill=tk.X, padx=8, pady=8)
            ttk.Label(top, text="Project folder:").pack(side=tk.LEFT)
            self.validate_dir_var = tk.StringVar()
            ttk.Entry(top, textvariable=self.validate_dir_var, width=50).pack(
                side=tk.LEFT, fill=tk.X, expand=True, padx=6)
            ttk.Button(top, text="Browse",
                       command=lambda: self._browse_dir(self.validate_dir_var)).pack(side=tk.LEFT)
            ttk.Button(top, text="Validate", command=self.do_validate).pack(side=tk.LEFT, padx=6)

            res_frame = ttk.LabelFrame(frame, text="Results", padding=6)
            res_frame.pack(fill=tk.BOTH, expand=True, padx=8, pady=4)
            self.validate_log = tk.Text(res_frame, height=22, font=("Consolas", 9), wrap=tk.NONE)
            vscroll = ttk.Scrollbar(res_frame, orient=tk.VERTICAL, command=self.validate_log.yview)
            self.validate_log.configure(yscrollcommand=vscroll.set)
            self.validate_log.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
            vscroll.pack(side=tk.RIGHT, fill=tk.Y)
            self.validate_log.tag_config("err", foreground="#b00020")
            self.validate_log.tag_config("ok", foreground="#0a7d28")
            self.validate_log.tag_config("warn", foreground="#a06000")

        def do_validate(self):
            self.validate_log.delete("1.0", tk.END)
            d = self.validate_dir_var.get().strip()
            if not d or not os.path.isdir(d):
                messagebox.showwarning("Validate", "Select a valid project folder first.")
                return
            results = validate_project(d)
            tagmap = {"ok": "ok", "warn": "warn", "error": "err"}
            iconmap = {"ok": "[ OK ] ", "warn": "[WARN] ", "error": "[FAIL] "}
            for lvl, msg in results:
                self.validate_log.insert(tk.END, iconmap[lvl] + msg + "\n", tagmap[lvl])
            errors = [m for lvl, m in results if lvl == "error"]
            self.validate_log.insert(tk.END, "\n")
            summary = "%d check(s), %d problem(s)." % (len(results), len(errors))
            self.validate_log.insert(tk.END, summary + "\n", "err" if errors else "ok")

        # ---------- Paths config ----------
        def edit_paths(self):
            paths = load_paths()
            dialog = tk.Toplevel(self.root)
            dialog.title("Dependency Paths (saved to paths.local.json)")
            dialog.geometry("720x480")
            dialog.transient(self.root)

            ttk.Label(dialog, foreground="gray",
                      text="These fill the generated .vcxproj. Double-click a value to edit. "
                           "Engine paths are auto-derived; overrides are saved separately.").pack(
                fill=tk.X, padx=8, pady=6)

            tree = ttk.Treeview(dialog, columns=("key", "value"), show="headings")
            tree.heading("key", text="Setting")
            tree.heading("value", text="Path")
            tree.column("key", width=220)
            tree.column("value", width=460)
            tree.pack(fill=tk.BOTH, expand=True, padx=8, pady=4)
            for k in sorted(paths.keys()):
                tree.insert("", tk.END, values=(k, paths[k]))

            def edit_value(event=None):
                sel = tree.selection()
                if not sel:
                    return
                key, value = tree.item(sel[0])["values"]
                d2 = tk.Toplevel(dialog)
                d2.title("Edit " + str(key))
                d2.geometry("560x120")
                d2.transient(dialog)
                d2.grab_set()
                ttk.Label(d2, text=str(key)).pack(pady=(10, 4))
                var = tk.StringVar(value=str(value))
                ttk.Entry(d2, textvariable=var, width=70).pack(padx=10, pady=4)

                def save_one():
                    tree.item(sel[0], values=(key, var.get()))
                    d2.destroy()
                ttk.Button(d2, text="OK", command=save_one).pack(pady=6)

            tree.bind("<Double-1>", edit_value)

            btns = ttk.Frame(dialog)
            btns.pack(fill=tk.X, padx=8, pady=8)

            def save_all():
                out = {}
                for item in tree.get_children():
                    k, val = tree.item(item)["values"]
                    out[str(k)] = str(val)
                save_paths_override(out)
                messagebox.showinfo("Saved", "Saved overrides to:\n" + PATHS_OVERRIDE_FILE)
                dialog.destroy()

            ttk.Button(btns, text="Save", command=save_all).pack(side=tk.RIGHT, padx=4)
            ttk.Button(btns, text="Cancel", command=dialog.destroy).pack(side=tk.RIGHT, padx=4)

    root = tk.Tk()
    NewProjectApp(root)
    root.mainloop()


if __name__ == "__main__":
    if len(sys.argv) > 1 and sys.argv[1] == "--cli":
        sys.exit(run_cli(sys.argv[2:]))
    else:
        main_gui()
