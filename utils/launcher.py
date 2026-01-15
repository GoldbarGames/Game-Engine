"""
Editor Launcher - Launch any of the config editor utilities
"""

import tkinter as tk
from tkinter import ttk
import subprocess
import sys
import os

class EditorLauncher:
    def __init__(self, root):
        self.root = root
        self.root.title("Game Engine Utils")
        self.root.geometry("400x400")
        self.root.resizable(False, False)

        # Get the directory where this script is located
        self.script_dir = os.path.dirname(os.path.abspath(__file__))

        self.create_ui()

    def create_ui(self):
        # Title
        title = ttk.Label(self.root, text="Game Engine Utilities", font=("Arial", 16, "bold"))
        title.pack(pady=15)

        # Editor buttons frame
        frame = ttk.LabelFrame(self.root, text="Select an Editor", padding=10)
        frame.pack(fill=tk.BOTH, expand=True, padx=20, pady=10)

        editors = [
            ("Config Editor", "configEditor.py", "Edit key=value config files (.dat, .config)"),
            ("Background Editor", "bgEditor.py", "Edit parallax backgrounds (bg.dat)"),
            ("List Editor", "listEditor.py", "Edit simple list files (one item per line)"),
            ("Animator Editor", "animEditor.py", "Edit animator state machines (.animator)"),
            ("Menu Editor", "menuEditor.py", "Visual menu layout designer"),
            ("GUI Editor", "guiEditor.py", "Visual HUD/GUI layout designer"),
        ]

        for name, script, desc in editors:
            btn_frame = ttk.Frame(frame)
            btn_frame.pack(fill=tk.X, pady=5)

            btn = ttk.Button(btn_frame, text=name, width=18,
                           command=lambda s=script: self.launch_editor(s))
            btn.pack(side=tk.LEFT, padx=5)

            lbl = ttk.Label(btn_frame, text=desc, foreground="gray")
            lbl.pack(side=tk.LEFT, padx=5)

        # Exit button
        ttk.Button(self.root, text="Exit", command=self.root.quit).pack(pady=15)

    def launch_editor(self, script_name):
        script_path = os.path.join(self.script_dir, script_name)
        if os.path.exists(script_path):
            subprocess.Popen([sys.executable, script_path], cwd=self.script_dir)
        else:
            tk.messagebox.showerror("Error", f"Script not found: {script_name}")


def main():
    root = tk.Tk()
    app = EditorLauncher(root)
    root.mainloop()


if __name__ == "__main__":
    main()
