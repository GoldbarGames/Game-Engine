"""
List Editor - GUI for editing entity type list files
Used for files in: data/lists/

- entityTypes.list defines all entity types
- Other .list files define subtypes for each entity type
- Format: one item per line
"""

import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import os


class ListEditor:
    def __init__(self, root):
        self.root = root
        self.root.title("List Editor")
        self.root.geometry("600x500")

        # Project data
        self.game_root = None
        self.list_files = []
        self.current_list_file = None

        # List data
        self.items = []
        self.modified = False

        self.create_menu()
        self.create_toolbar()
        self.create_main_area()
        self.create_status_bar()

    def create_menu(self):
        menubar = tk.Menu(self.root)
        self.root.config(menu=menubar)

        file_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="File", menu=file_menu)
        file_menu.add_command(label="Open Project...", command=self.open_project, accelerator="Ctrl+O")
        file_menu.add_separator()
        file_menu.add_command(label="New List...", command=self.new_list, accelerator="Ctrl+N")
        file_menu.add_separator()
        file_menu.add_command(label="Save", command=self.save_current, accelerator="Ctrl+S")
        file_menu.add_separator()
        file_menu.add_command(label="Exit", command=self.on_close)

        self.root.bind("<Control-o>", lambda e: self.open_project())
        self.root.bind("<Control-n>", lambda e: self.new_list())
        self.root.bind("<Control-s>", lambda e: self.save_current())
        self.root.bind("<Delete>", lambda e: self.delete_item())

    def create_toolbar(self):
        toolbar = ttk.Frame(self.root)
        toolbar.pack(fill=tk.X, padx=5, pady=5)

        ttk.Button(toolbar, text="Open Project", command=self.open_project).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="Save", command=self.save_current).pack(side=tk.LEFT, padx=2)

        ttk.Separator(toolbar, orient=tk.VERTICAL).pack(side=tk.LEFT, fill=tk.Y, padx=10)

        ttk.Label(toolbar, text="List:").pack(side=tk.LEFT, padx=5)
        self.list_file_var = tk.StringVar()
        self.list_file_combo = ttk.Combobox(toolbar, textvariable=self.list_file_var,
                                             state="readonly", width=25)
        self.list_file_combo.pack(side=tk.LEFT, padx=5)
        self.list_file_combo.bind("<<ComboboxSelected>>", self.on_list_file_selected)

        # Project path label
        ttk.Separator(toolbar, orient=tk.VERTICAL).pack(side=tk.LEFT, fill=tk.Y, padx=10)
        self.project_path_var = tk.StringVar(value="No project open")
        ttk.Label(toolbar, textvariable=self.project_path_var, foreground="gray").pack(side=tk.LEFT, padx=5)

    def create_main_area(self):
        main_frame = ttk.Frame(self.root)
        main_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        # Left side - list items
        list_frame = ttk.LabelFrame(main_frame, text="Items")
        list_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(0, 5))

        # Buttons above list
        btn_frame = ttk.Frame(list_frame)
        btn_frame.pack(fill=tk.X, padx=5, pady=5)
        ttk.Button(btn_frame, text="Add", command=self.add_item).pack(side=tk.LEFT, padx=2)
        ttk.Button(btn_frame, text="Delete", command=self.delete_item).pack(side=tk.LEFT, padx=2)
        ttk.Button(btn_frame, text="Up", command=self.move_up).pack(side=tk.LEFT, padx=2)
        ttk.Button(btn_frame, text="Down", command=self.move_down).pack(side=tk.LEFT, padx=2)

        # Listbox
        listbox_frame = ttk.Frame(list_frame)
        listbox_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=(0, 5))

        self.listbox = tk.Listbox(listbox_frame, selectmode=tk.SINGLE, font=("Consolas", 11),
                                   exportselection=False)
        scrollbar = ttk.Scrollbar(listbox_frame, orient=tk.VERTICAL, command=self.listbox.yview)
        self.listbox.configure(yscrollcommand=scrollbar.set)

        self.listbox.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

        self.listbox.bind("<Double-1>", self.edit_item)

        # Right side - edit panel
        edit_frame = ttk.LabelFrame(main_frame, text="Edit Item", width=200)
        edit_frame.pack(side=tk.RIGHT, fill=tk.Y, padx=(5, 0))
        edit_frame.pack_propagate(False)

        ttk.Label(edit_frame, text="Name:").pack(anchor=tk.W, padx=10, pady=(10, 5))
        self.item_entry = ttk.Entry(edit_frame, width=25)
        self.item_entry.pack(padx=10, pady=5)
        self.item_entry.bind("<Return>", lambda e: self.save_item())

        btn_frame2 = ttk.Frame(edit_frame)
        btn_frame2.pack(pady=10)
        ttk.Button(btn_frame2, text="Add", command=self.add_item).pack(side=tk.LEFT, padx=5)
        ttk.Button(btn_frame2, text="Save", command=self.save_item).pack(side=tk.LEFT, padx=5)
        ttk.Button(btn_frame2, text="Clear", command=self.clear_entry).pack(side=tk.LEFT, padx=5)

    def create_status_bar(self):
        self.status_var = tk.StringVar(value="Ready - Open a project to begin")
        status_bar = ttk.Label(self.root, textvariable=self.status_var, relief=tk.SUNKEN, anchor=tk.W)
        status_bar.pack(side=tk.BOTTOM, fill=tk.X)

    # ============== Project Operations ==============

    def normalize_path(self, path):
        """Normalize path to use forward slashes consistently."""
        return path.replace("\\", "/")

    def open_project(self):
        """Open a game project folder."""
        if not self.check_unsaved_changes():
            return

        folder = filedialog.askdirectory(
            title="Select Game Root Folder",
            mustexist=True
        )
        if folder:
            # Verify this looks like a game project
            lists_path = os.path.join(folder, "data", "lists")
            if not os.path.exists(lists_path):
                if messagebox.askyesno("Create Folder?",
                    "No 'data/lists' folder found.\n\nCreate it?"):
                    os.makedirs(lists_path, exist_ok=True)
                else:
                    return

            self.game_root = folder
            self.project_path_var.set(self.normalize_path(folder))
            self.scan_list_files()
            self.update_title()
            self.status_var.set(f"Opened project: {self.normalize_path(folder)}")

    def scan_list_files(self):
        """Scan the data/lists folder for .list files."""
        self.list_files = []
        lists_path = os.path.join(self.game_root, "data", "lists")

        if os.path.exists(lists_path):
            for item in sorted(os.listdir(lists_path)):
                if item.endswith('.list'):
                    self.list_files.append(item)

        self.list_file_combo['values'] = self.list_files

        # Clear current data
        self.clear_data()

        if self.list_files:
            # Try to select entityTypes.list first, otherwise first file
            if 'entityTypes.list' in self.list_files:
                self.list_file_combo.set('entityTypes.list')
                self.load_list_file('entityTypes.list')
            else:
                self.list_file_combo.set(self.list_files[0])
                self.load_list_file(self.list_files[0])
        else:
            self.list_file_combo.set("")
            self.status_var.set("No list files found - create one with File > New List")

    def on_list_file_selected(self, event=None):
        """Handle list file selection change."""
        if not self.check_unsaved_changes():
            # Revert to previous selection
            if self.current_list_file:
                self.list_file_combo.set(self.current_list_file)
            return

        selected = self.list_file_var.get()
        if selected and selected != self.current_list_file:
            self.load_list_file(selected)

    def load_list_file(self, filename):
        """Load a list file."""
        self.clear_data()
        self.current_list_file = filename

        filepath = os.path.join(self.game_root, "data", "lists", filename)

        try:
            with open(filepath, 'r') as f:
                for line in f:
                    line = line.strip()
                    if line:
                        self.items.append(line)

            self.refresh_listbox()
            self.modified = False
            self.update_title()
            self.status_var.set(f"Loaded: {filename} ({len(self.items)} items)")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to load file:\n{e}")

    def clear_data(self):
        """Clear current list data."""
        self.items = []
        self.current_list_file = None
        self.modified = False
        self.refresh_listbox()
        self.clear_entry()

    def new_list(self):
        """Create a new list file."""
        if not self.game_root:
            messagebox.showwarning("Warning", "Please open a project first")
            return

        if not self.check_unsaved_changes():
            return

        dialog = tk.Toplevel(self.root)
        dialog.title("New List")
        dialog.geometry("300x120")
        dialog.transient(self.root)
        dialog.grab_set()

        ttk.Label(dialog, text="List Name (without .list):").pack(pady=10)
        name_entry = ttk.Entry(dialog, width=25)
        name_entry.pack(pady=5)
        name_entry.focus_set()

        def do_create():
            name = name_entry.get().strip()
            if not name:
                return
            filename = f"{name}.list"
            if filename in self.list_files:
                messagebox.showwarning("Warning", f"List '{filename}' already exists")
                return

            # Create empty file
            filepath = os.path.join(self.game_root, "data", "lists", filename)
            with open(filepath, 'w') as f:
                f.write("")

            dialog.destroy()

            # Refresh and select new file
            self.scan_list_files()
            if filename in self.list_files:
                self.list_file_combo.set(filename)
                self.load_list_file(filename)

            self.status_var.set(f"Created: {filename}")

        name_entry.bind("<Return>", lambda e: do_create())
        ttk.Button(dialog, text="Create", command=do_create).pack(pady=10)

    def save_current(self):
        """Save current list file."""
        if not self.current_list_file:
            messagebox.showwarning("Warning", "No list file selected")
            return

        filepath = os.path.join(self.game_root, "data", "lists", self.current_list_file)

        try:
            with open(filepath, 'w') as f:
                for item in self.items:
                    f.write(f"{item}\n")

            self.modified = False
            self.update_title()
            self.status_var.set(f"Saved: {self.current_list_file}")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to save file:\n{e}")

    def check_unsaved_changes(self):
        """Check for unsaved changes and prompt user. Returns True if OK to proceed."""
        if self.modified:
            result = messagebox.askyesnocancel(
                "Unsaved Changes",
                f"Save changes to '{self.current_list_file}'?"
            )
            if result is None:  # Cancel
                return False
            if result:  # Yes
                self.save_current()
        return True

    # ============== List Operations ==============

    def refresh_listbox(self):
        """Refresh the listbox display."""
        self.listbox.delete(0, tk.END)
        for item in self.items:
            self.listbox.insert(tk.END, item)

    def add_item(self):
        """Add a new item from the entry field."""
        value = self.item_entry.get().strip()
        if not value:
            return

        if value in self.items:
            messagebox.showwarning("Warning", f"Item '{value}' already exists")
            return

        self.items.append(value)
        self.refresh_listbox()
        self.clear_entry()
        self.modified = True
        self.update_title()

        # Select the newly added item
        self.listbox.selection_clear(0, tk.END)
        self.listbox.selection_set(tk.END)
        self.listbox.see(tk.END)
        self.status_var.set(f"Added: {value}")

    def edit_item(self, event=None):
        """Load selected item into entry field for editing."""
        selection = self.listbox.curselection()
        if selection:
            index = selection[0]
            self.item_entry.delete(0, tk.END)
            self.item_entry.insert(0, self.items[index])
            self.item_entry.focus_set()

    def save_item(self):
        """Save edits to the selected item."""
        selection = self.listbox.curselection()
        if not selection:
            # No selection, treat as add
            self.add_item()
            return

        value = self.item_entry.get().strip()
        if not value:
            return

        index = selection[0]
        old_value = self.items[index]

        # Check for duplicate (but allow same value for same item)
        if value != old_value and value in self.items:
            messagebox.showwarning("Warning", f"Item '{value}' already exists")
            return

        self.items[index] = value
        self.refresh_listbox()
        self.listbox.selection_set(index)
        self.modified = True
        self.update_title()
        self.status_var.set(f"Updated: {old_value} -> {value}")

    def delete_item(self):
        """Delete the selected item."""
        selection = self.listbox.curselection()
        if not selection:
            return

        index = selection[0]
        item = self.items[index]

        if messagebox.askyesno("Confirm", f"Delete '{item}'?"):
            del self.items[index]
            self.refresh_listbox()
            self.modified = True
            self.update_title()
            self.status_var.set(f"Deleted: {item}")

            # Select next item if available
            if self.items:
                new_index = min(index, len(self.items) - 1)
                self.listbox.selection_set(new_index)

    def move_up(self):
        """Move selected item up."""
        selection = self.listbox.curselection()
        if selection and selection[0] > 0:
            index = selection[0]
            self.items[index], self.items[index - 1] = self.items[index - 1], self.items[index]
            self.refresh_listbox()
            self.listbox.selection_set(index - 1)
            self.modified = True
            self.update_title()

    def move_down(self):
        """Move selected item down."""
        selection = self.listbox.curselection()
        if selection and selection[0] < len(self.items) - 1:
            index = selection[0]
            self.items[index], self.items[index + 1] = self.items[index + 1], self.items[index]
            self.refresh_listbox()
            self.listbox.selection_set(index + 1)
            self.modified = True
            self.update_title()

    def clear_entry(self):
        """Clear the entry field."""
        self.item_entry.delete(0, tk.END)
        self.listbox.selection_clear(0, tk.END)

    # ============== Common Methods ==============

    def update_title(self):
        title = "List Editor"
        if self.game_root:
            project_name = os.path.basename(self.game_root)
            title += f" - {project_name}"
            if self.current_list_file:
                modified = "*" if self.modified else ""
                title += f" / {self.current_list_file}{modified}"
        self.root.title(title)

    def on_close(self):
        if not self.check_unsaved_changes():
            return
        self.root.destroy()


def main():
    root = tk.Tk()
    app = ListEditor(root)
    root.protocol("WM_DELETE_WINDOW", app.on_close)
    root.mainloop()


if __name__ == "__main__":
    main()
