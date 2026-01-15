"""
Animator Editor - GUI for editing animator files
Used for files in: data/animators/

Two file types:
1. *.animator - State machine transitions
   Format: *stateName* followed by transition lines

2. animations - Animation definitions (sprite data)
   Format: name speed startFrame endFrame frameWidth frameHeight filepath pivotX pivotY
"""

import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import os

try:
    from PIL import Image, ImageTk
    HAS_PIL = True
except ImportError:
    HAS_PIL = False
    print("PIL not found - animation preview disabled. Install with: pip install Pillow")

# ============== State Machine Classes ==============

class AnimCondition:
    def __init__(self, var_type="bool", var_name="", operator="=", value="true"):
        self.var_type = var_type
        self.var_name = var_name
        self.operator = operator
        self.value = value

    def __str__(self):
        return f"{self.var_type} {self.var_name} {self.operator} {self.value}"

class AnimTransition:
    def __init__(self, target_state="", conditions=None):
        self.target_state = target_state
        self.conditions = conditions if conditions else []

    def to_string(self):
        cond_str = " && ".join(str(c) for c in self.conditions)
        return f"{self.target_state}: {cond_str}"

class AnimState:
    def __init__(self, name=""):
        self.name = name
        self.transitions = []

# ============== Animation Definition Class ==============

class AnimationDef:
    def __init__(self, name="", speed=100, start_frame=0, end_frame=0,
                 frame_width=32, frame_height=32, filepath="", pivot_x=0, pivot_y=0):
        self.name = name
        self.speed = speed
        self.start_frame = start_frame
        self.end_frame = end_frame
        self.frame_width = frame_width
        self.frame_height = frame_height
        self.filepath = filepath
        self.pivot_x = pivot_x
        self.pivot_y = pivot_y

    def to_line(self):
        return f"{self.name} {self.speed} {self.start_frame} {self.end_frame} {self.frame_width} {self.frame_height} {self.filepath} {self.pivot_x} {self.pivot_y}"

    @staticmethod
    def from_line(line):
        tokens = line.split()
        if len(tokens) >= 9:
            return AnimationDef(
                name=tokens[0],
                speed=int(tokens[1]),
                start_frame=int(tokens[2]),
                end_frame=int(tokens[3]),
                frame_width=int(tokens[4]),
                frame_height=int(tokens[5]),
                filepath=tokens[6],
                pivot_x=int(tokens[7]),
                pivot_y=int(tokens[8])
            )
        return None

# ============== Main Editor ==============

class AnimatorEditor:
    def __init__(self, root):
        self.root = root
        self.root.title("Animator Editor")
        self.root.geometry("1100x750")

        # Project data
        self.game_root = None
        self.entity_types = []
        self.current_entity_type = None

        # State machine data
        self.sm_current_file = None
        self.states = {}
        self.state_order = []
        self.sm_modified = False
        self.selected_state_name = None

        # Animation definitions data
        self.anim_current_file = None
        self.animations = []
        self.anim_modified = False
        self.selected_anim_index = None

        # Animation preview data
        self.preview_frames = []
        self.preview_frame_index = 0
        self.preview_after_id = None
        self.preview_photo = None  # Keep reference to prevent garbage collection

        # List parameter data (for {1}, {2}, etc. substitution in filepaths)
        self.list_items = []  # Items from <entity_type>.list

        self.create_menu()
        self.create_toolbar()
        self.create_notebook()
        self.create_status_bar()

    def create_menu(self):
        menubar = tk.Menu(self.root)
        self.root.config(menu=menubar)

        file_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="File", menu=file_menu)
        file_menu.add_command(label="Open Project...", command=self.open_project, accelerator="Ctrl+O")
        file_menu.add_separator()
        file_menu.add_command(label="New Entity Type...", command=self.new_entity_type, accelerator="Ctrl+N")
        file_menu.add_separator()
        file_menu.add_command(label="Save", command=self.save_current, accelerator="Ctrl+S")
        file_menu.add_command(label="Save All", command=self.save_all, accelerator="Ctrl+Shift+S")
        file_menu.add_separator()
        file_menu.add_command(label="Exit", command=self.on_close)

        self.root.bind("<Control-o>", lambda e: self.open_project())
        self.root.bind("<Control-n>", lambda e: self.new_entity_type())
        self.root.bind("<Control-s>", lambda e: self.save_current())
        self.root.bind("<Control-S>", lambda e: self.save_all())

    def create_toolbar(self):
        toolbar = ttk.Frame(self.root)
        toolbar.pack(fill=tk.X, padx=5, pady=5)

        ttk.Button(toolbar, text="Open Project", command=self.open_project).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="Save", command=self.save_current).pack(side=tk.LEFT, padx=2)

        ttk.Separator(toolbar, orient=tk.VERTICAL).pack(side=tk.LEFT, fill=tk.Y, padx=10)

        ttk.Label(toolbar, text="Entity Type:").pack(side=tk.LEFT, padx=5)
        self.entity_type_var = tk.StringVar()
        self.entity_type_combo = ttk.Combobox(toolbar, textvariable=self.entity_type_var,
                                               state="readonly", width=25)
        self.entity_type_combo.pack(side=tk.LEFT, padx=5)
        self.entity_type_combo.bind("<<ComboboxSelected>>", self.on_entity_type_selected)

        # Project path label
        ttk.Separator(toolbar, orient=tk.VERTICAL).pack(side=tk.LEFT, fill=tk.Y, padx=10)
        self.project_path_var = tk.StringVar(value="No project open")
        ttk.Label(toolbar, textvariable=self.project_path_var, foreground="gray").pack(side=tk.LEFT, padx=5)

    def create_notebook(self):
        self.notebook = ttk.Notebook(self.root)
        self.notebook.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        # Tab 1: State Machine
        self.sm_frame = ttk.Frame(self.notebook)
        self.notebook.add(self.sm_frame, text="State Machine (.animator)")
        self.create_state_machine_tab()

        # Tab 2: Animations
        self.anim_frame = ttk.Frame(self.notebook)
        self.notebook.add(self.anim_frame, text="Animations (animations)")
        self.create_animations_tab()

    # ============== State Machine Tab ==============

    def create_state_machine_tab(self):
        paned = ttk.PanedWindow(self.sm_frame, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        # Left panel - States list
        left_frame = ttk.LabelFrame(paned, text="States")
        paned.add(left_frame, weight=1)

        state_buttons = ttk.Frame(left_frame)
        state_buttons.pack(side=tk.TOP, fill=tk.X)
        ttk.Button(state_buttons, text="Add", command=self.add_state).pack(side=tk.LEFT, padx=2, pady=2)
        ttk.Button(state_buttons, text="Delete", command=self.delete_state).pack(side=tk.LEFT, padx=2, pady=2)
        ttk.Button(state_buttons, text="Rename", command=self.rename_state).pack(side=tk.LEFT, padx=2, pady=2)

        list_frame = ttk.Frame(left_frame)
        list_frame.pack(side=tk.TOP, fill=tk.BOTH, expand=True)

        self.state_listbox = tk.Listbox(list_frame, selectmode=tk.SINGLE, font=("Consolas", 10), exportselection=False)
        state_scroll = ttk.Scrollbar(list_frame, orient=tk.VERTICAL, command=self.state_listbox.yview)
        self.state_listbox.configure(yscrollcommand=state_scroll.set)
        self.state_listbox.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        state_scroll.pack(side=tk.RIGHT, fill=tk.Y)
        self.state_listbox.bind("<ButtonRelease-1>", self.on_state_click)

        # Right panel - Transitions
        right_frame = ttk.Frame(paned)
        paned.add(right_frame, weight=2)

        # Transition editor
        edit_frame = ttk.LabelFrame(right_frame, text="Edit Transition")
        edit_frame.pack(fill=tk.X, pady=5)

        row1 = ttk.Frame(edit_frame)
        row1.pack(fill=tk.X, pady=2)
        ttk.Label(row1, text="Target State:").pack(side=tk.LEFT, padx=5)
        self.target_combo = ttk.Combobox(row1, width=20)
        self.target_combo.pack(side=tk.LEFT, padx=5)

        cond_frame = ttk.LabelFrame(edit_frame, text="Conditions")
        cond_frame.pack(fill=tk.X, padx=5, pady=5)

        self.cond_entries = []
        for i in range(3):
            cond_row = ttk.Frame(cond_frame)
            cond_row.pack(fill=tk.X, pady=2)

            if i > 0:
                ttk.Label(cond_row, text="&&").pack(side=tk.LEFT, padx=5)

            type_combo = ttk.Combobox(cond_row, values=["bool", "int", "float"], width=6)
            type_combo.set("bool")
            type_combo.pack(side=tk.LEFT, padx=2)

            name_entry = ttk.Entry(cond_row, width=15)
            name_entry.pack(side=tk.LEFT, padx=2)

            op_combo = ttk.Combobox(cond_row, values=["=", "!=", "<", ">", "<=", ">="], width=4)
            op_combo.set("=")
            op_combo.pack(side=tk.LEFT, padx=2)

            value_entry = ttk.Entry(cond_row, width=10)
            value_entry.pack(side=tk.LEFT, padx=2)

            self.cond_entries.append((type_combo, name_entry, op_combo, value_entry))

        btn_row = ttk.Frame(edit_frame)
        btn_row.pack(fill=tk.X, pady=5)
        ttk.Button(btn_row, text="Add", command=self.add_transition).pack(side=tk.LEFT, padx=5)
        ttk.Button(btn_row, text="Save", command=self.save_transition).pack(side=tk.LEFT, padx=5)
        ttk.Button(btn_row, text="Clear", command=self.clear_transition_form).pack(side=tk.LEFT, padx=5)

        # Transitions list
        trans_frame = ttk.LabelFrame(right_frame, text="Transitions from Selected State")
        trans_frame.pack(fill=tk.BOTH, expand=True)

        trans_buttons = ttk.Frame(trans_frame)
        trans_buttons.pack(side=tk.TOP, fill=tk.X)
        ttk.Button(trans_buttons, text="Edit", command=self.edit_transition).pack(side=tk.LEFT, padx=2, pady=2)
        ttk.Button(trans_buttons, text="Delete", command=self.delete_transition).pack(side=tk.LEFT, padx=2, pady=2)

        tree_frame = ttk.Frame(trans_frame)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        columns = ("target", "conditions")
        self.trans_tree = ttk.Treeview(tree_frame, columns=columns, show="headings", selectmode="browse")
        self.trans_tree.heading("target", text="Target State", anchor=tk.W)
        self.trans_tree.heading("conditions", text="Conditions", anchor=tk.W)
        self.trans_tree.column("target", width=120)
        self.trans_tree.column("conditions", width=400)

        trans_scroll = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.trans_tree.yview)
        self.trans_tree.configure(yscrollcommand=trans_scroll.set)
        self.trans_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        trans_scroll.pack(side=tk.RIGHT, fill=tk.Y)

        self.trans_tree.bind("<Double-1>", self.edit_transition)

    # ============== Animations Tab ==============

    def create_animations_tab(self):
        # Top - Edit form
        edit_frame = ttk.LabelFrame(self.anim_frame, text="Edit Animation")
        edit_frame.pack(fill=tk.X, padx=5, pady=5)

        # Row 1: All parameters except filepath
        row1 = ttk.Frame(edit_frame)
        row1.pack(fill=tk.X, pady=2)

        ttk.Label(row1, text="Name:").pack(side=tk.LEFT, padx=(5,2))
        self.anim_name_entry = ttk.Entry(row1, width=12)
        self.anim_name_entry.pack(side=tk.LEFT, padx=2)

        ttk.Label(row1, text="Speed:").pack(side=tk.LEFT, padx=(5,2))
        self.anim_speed_entry = ttk.Entry(row1, width=5)
        self.anim_speed_entry.pack(side=tk.LEFT, padx=2)

        ttk.Label(row1, text="Start:").pack(side=tk.LEFT, padx=(5,2))
        self.anim_start_entry = ttk.Entry(row1, width=4)
        self.anim_start_entry.pack(side=tk.LEFT, padx=2)

        ttk.Label(row1, text="End:").pack(side=tk.LEFT, padx=(5,2))
        self.anim_end_entry = ttk.Entry(row1, width=4)
        self.anim_end_entry.pack(side=tk.LEFT, padx=2)

        ttk.Label(row1, text="W:").pack(side=tk.LEFT, padx=(5,2))
        self.anim_width_entry = ttk.Entry(row1, width=4)
        self.anim_width_entry.pack(side=tk.LEFT, padx=2)

        ttk.Label(row1, text="H:").pack(side=tk.LEFT, padx=(5,2))
        self.anim_height_entry = ttk.Entry(row1, width=4)
        self.anim_height_entry.pack(side=tk.LEFT, padx=2)

        ttk.Label(row1, text="Pivot X:").pack(side=tk.LEFT, padx=(5,2))
        self.anim_pivotx_entry = ttk.Entry(row1, width=4)
        self.anim_pivotx_entry.pack(side=tk.LEFT, padx=2)

        ttk.Label(row1, text="Y:").pack(side=tk.LEFT, padx=(5,2))
        self.anim_pivoty_entry = ttk.Entry(row1, width=4)
        self.anim_pivoty_entry.pack(side=tk.LEFT, padx=2)

        # Row 2: Filepath and browse button
        row2 = ttk.Frame(edit_frame)
        row2.pack(fill=tk.X, pady=2)

        ttk.Label(row2, text="Filepath:").pack(side=tk.LEFT, padx=5)
        self.anim_filepath_entry = ttk.Entry(row2, width=50)
        self.anim_filepath_entry.pack(side=tk.LEFT, padx=5, fill=tk.X, expand=True)
        ttk.Button(row2, text="Browse...", command=self.browse_sprite_file).pack(side=tk.LEFT, padx=5)
        ttk.Button(row2, text="Preview", command=self.preview_from_form).pack(side=tk.LEFT, padx=5)

        # Buttons
        btn_row = ttk.Frame(edit_frame)
        btn_row.pack(fill=tk.X, pady=5)
        ttk.Button(btn_row, text="Add", command=self.add_animation).pack(side=tk.LEFT, padx=5)
        ttk.Button(btn_row, text="Save", command=self.save_animation).pack(side=tk.LEFT, padx=5)
        ttk.Button(btn_row, text="Clear", command=self.clear_animation_form).pack(side=tk.LEFT, padx=5)

        # Middle - Paned window for list and preview
        paned = ttk.PanedWindow(self.anim_frame, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        # Left - Animations list
        list_frame = ttk.LabelFrame(paned, text="Animations")
        paned.add(list_frame, weight=3)

        # Buttons
        list_buttons = ttk.Frame(list_frame)
        list_buttons.pack(side=tk.TOP, fill=tk.X)
        ttk.Button(list_buttons, text="Edit", command=self.edit_animation).pack(side=tk.LEFT, padx=2, pady=2)
        ttk.Button(list_buttons, text="Delete", command=self.delete_animation).pack(side=tk.LEFT, padx=2, pady=2)

        # Treeview
        tree_frame = ttk.Frame(list_frame)
        tree_frame.pack(fill=tk.BOTH, expand=True)

        columns = ("name", "speed", "frames", "size", "filepath", "pivot")
        self.anim_tree = ttk.Treeview(tree_frame, columns=columns, show="headings", selectmode="browse")
        self.anim_tree.heading("name", text="Name", anchor=tk.W)
        self.anim_tree.heading("speed", text="Speed", anchor=tk.W)
        self.anim_tree.heading("frames", text="Frames", anchor=tk.W)
        self.anim_tree.heading("size", text="Size", anchor=tk.W)
        self.anim_tree.heading("filepath", text="Filepath", anchor=tk.W)
        self.anim_tree.heading("pivot", text="Pivot", anchor=tk.W)

        self.anim_tree.column("name", width=100)
        self.anim_tree.column("speed", width=60)
        self.anim_tree.column("frames", width=80)
        self.anim_tree.column("size", width=80)
        self.anim_tree.column("filepath", width=200)
        self.anim_tree.column("pivot", width=80)

        anim_scroll = ttk.Scrollbar(tree_frame, orient=tk.VERTICAL, command=self.anim_tree.yview)
        self.anim_tree.configure(yscrollcommand=anim_scroll.set)
        self.anim_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        anim_scroll.pack(side=tk.RIGHT, fill=tk.Y)

        self.anim_tree.bind("<Double-1>", lambda e: self.edit_animation())
        self.anim_tree.bind("<<TreeviewSelect>>", self.on_animation_select)

        # Right - Preview panel
        preview_frame = ttk.LabelFrame(paned, text="Preview")
        paned.add(preview_frame, weight=1)

        # Variant selector (for {1} substitution)
        variant_frame = ttk.Frame(preview_frame)
        variant_frame.pack(fill=tk.X, padx=5, pady=5)
        ttk.Label(variant_frame, text="Variant {1}:").pack(side=tk.LEFT, padx=2)
        self.variant_var = tk.StringVar()
        self.variant_combo = ttk.Combobox(variant_frame, textvariable=self.variant_var,
                                          state="readonly", width=15)
        self.variant_combo.pack(side=tk.LEFT, padx=2, fill=tk.X, expand=True)
        self.variant_combo.bind("<<ComboboxSelected>>", self.on_variant_selected)

        # Preview canvas with background
        self.preview_canvas = tk.Canvas(preview_frame, width=200, height=200, bg="#333333")
        self.preview_canvas.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        # Preview info label
        self.preview_info_var = tk.StringVar(value="Select an animation to preview")
        ttk.Label(preview_frame, textvariable=self.preview_info_var).pack(pady=5)

    def create_status_bar(self):
        self.status_var = tk.StringVar(value="Ready - Open a project to begin")
        status_bar = ttk.Label(self.root, textvariable=self.status_var, relief=tk.SUNKEN, anchor=tk.W)
        status_bar.pack(side=tk.BOTTOM, fill=tk.X)

    # ============== Project Operations ==============

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
            animators_path = os.path.join(folder, "data", "animators")
            if not os.path.exists(animators_path):
                if messagebox.askyesno("Create Folder?",
                    f"No 'data/animators' folder found.\n\nCreate it?"):
                    os.makedirs(animators_path, exist_ok=True)
                else:
                    return

            self.game_root = folder
            self.project_path_var.set(folder)
            self.scan_entity_types()
            self.update_title()
            self.status_var.set(f"Opened project: {folder}")

    def scan_entity_types(self):
        """Scan the data/animators folder for entity types."""
        self.entity_types = []
        animators_path = os.path.join(self.game_root, "data", "animators")

        if os.path.exists(animators_path):
            for item in sorted(os.listdir(animators_path)):
                item_path = os.path.join(animators_path, item)
                if os.path.isdir(item_path):
                    self.entity_types.append(item)

        self.entity_type_combo['values'] = self.entity_types

        # Clear current data
        self.clear_all_data()

        if self.entity_types:
            self.entity_type_combo.set(self.entity_types[0])
            self.load_entity_type(self.entity_types[0])
        else:
            self.entity_type_combo.set("")
            self.status_var.set("No entity types found - create one with File > New Entity Type")

    def on_entity_type_selected(self, event=None):
        """Handle entity type selection change."""
        if not self.check_unsaved_changes():
            # Revert to previous selection
            if self.current_entity_type:
                self.entity_type_combo.set(self.current_entity_type)
            return

        selected = self.entity_type_var.get()
        if selected and selected != self.current_entity_type:
            self.load_entity_type(selected)

    def normalize_path(self, path):
        """Normalize path to use forward slashes consistently."""
        return path.replace("\\", "/")

    def load_entity_type(self, entity_type):
        """Load all animation data for an entity type."""
        self.clear_all_data()
        self.current_entity_type = entity_type

        entity_path = os.path.join(self.game_root, "data", "animators", entity_type)
        loaded_files = []

        # Load .animator file (state machine)
        animator_files = [f for f in os.listdir(entity_path) if f.endswith('.animator')]
        if animator_files:
            # Use the first .animator file found (usually matches entity type name)
            self.sm_current_file = os.path.join(entity_path, animator_files[0])
            display_path = self.normalize_path(self.sm_current_file)
            self.status_var.set(f"Loading: {display_path}")
            self.root.update()
            self.load_state_machine(self.sm_current_file)
            loaded_files.append(f"{display_path} ({len(self.states)} states)")
        else:
            loaded_files.append(f"{self.normalize_path(entity_path)}: No .animator file found")

        # Load animations file (<entity_type>.animations)
        animations_file = os.path.join(entity_path, f"{entity_type}.animations")
        if os.path.exists(animations_file):
            self.anim_current_file = animations_file
            display_path = self.normalize_path(animations_file)
            self.status_var.set(f"Loading: {display_path}")
            self.root.update()
            self.load_animations(animations_file)
            loaded_files.append(f"{display_path} ({len(self.animations)} anims)")
        else:
            loaded_files.append(f"{self.normalize_path(animations_file)}: Not found")

        # Load list file for {1} parameter substitution
        self.load_entity_list(entity_type)

        self.update_title()
        self.status_var.set(f"Loaded: {' | '.join(loaded_files)}")

    def load_entity_list(self, entity_type):
        """Load the list file for parameter substitution (e.g., {1})."""
        self.list_items = []
        list_file = os.path.join(self.game_root, "data", "lists", f"{entity_type}.list")

        if os.path.exists(list_file):
            try:
                with open(list_file, 'r') as f:
                    for line in f:
                        line = line.strip()
                        if line:
                            self.list_items.append(line)
            except Exception as e:
                print(f"Error loading list file: {e}")

        # Update variant dropdown
        if hasattr(self, 'variant_combo'):
            self.variant_combo['values'] = self.list_items
            if self.list_items:
                self.variant_combo.set(self.list_items[0])
            else:
                self.variant_combo.set("")

    def on_variant_selected(self, event=None):
        """Refresh preview when variant selection changes."""
        anim = self.get_selected_animation()
        if anim:
            self.start_preview(anim)

    def clear_all_data(self):
        """Clear all loaded data."""
        self.stop_preview()

        # Clear state machine
        self.sm_current_file = None
        self.states = {}
        self.state_order = []
        self.selected_state_name = None
        self.sm_modified = False
        self.refresh_states()
        self.refresh_transitions()

        # Clear animations
        self.anim_current_file = None
        self.animations = []
        self.selected_anim_index = None
        self.anim_modified = False
        self.refresh_animations()
        self.clear_animation_form()

        # Clear list items
        self.list_items = []
        if hasattr(self, 'variant_combo'):
            self.variant_combo['values'] = []
            self.variant_combo.set("")

        self.current_entity_type = None

    def new_entity_type(self):
        """Create a new entity type folder."""
        if not self.game_root:
            messagebox.showwarning("Warning", "Please open a project first")
            return

        if not self.check_unsaved_changes():
            return

        dialog = tk.Toplevel(self.root)
        dialog.title("New Entity Type")
        dialog.geometry("350x120")
        dialog.transient(self.root)
        dialog.grab_set()

        ttk.Label(dialog, text="Entity Type Name:").pack(pady=10)
        name_entry = ttk.Entry(dialog, width=30)
        name_entry.pack(pady=5)
        name_entry.focus_set()

        def do_create():
            name = name_entry.get().strip()
            if not name:
                return
            if name in self.entity_types:
                messagebox.showwarning("Warning", f"Entity type '{name}' already exists")
                return

            # Create folder and empty files
            entity_path = os.path.join(self.game_root, "data", "animators", name)
            os.makedirs(entity_path, exist_ok=True)

            # Create empty .animator file
            animator_file = os.path.join(entity_path, f"{name}.animator")
            with open(animator_file, 'w') as f:
                f.write("")

            # Create empty .animations file
            animations_file = os.path.join(entity_path, f"{name}.animations")
            with open(animations_file, 'w') as f:
                f.write("")

            dialog.destroy()

            # Refresh and select new entity type
            self.scan_entity_types()
            if name in self.entity_types:
                self.entity_type_combo.set(name)
                self.load_entity_type(name)

            self.status_var.set(f"Created entity type: {name}")

        name_entry.bind("<Return>", lambda e: do_create())
        ttk.Button(dialog, text="Create", command=do_create).pack(pady=10)

    def save_current(self):
        """Save current entity type's files."""
        if not self.current_entity_type:
            messagebox.showwarning("Warning", "No entity type selected")
            return

        entity_path = os.path.join(self.game_root, "data", "animators", self.current_entity_type)

        # Save state machine
        if self.sm_modified:
            if not self.sm_current_file:
                self.sm_current_file = os.path.join(entity_path, f"{self.current_entity_type}.animator")
            self.write_state_machine(self.sm_current_file)

        # Save animations
        if self.anim_modified:
            if not self.anim_current_file:
                self.anim_current_file = os.path.join(entity_path, f"{self.current_entity_type}.animations")
            self.write_animations(self.anim_current_file)

        self.status_var.set(f"Saved: {self.current_entity_type}")

    def save_all(self):
        """Save all modified files."""
        self.save_current()

    def check_unsaved_changes(self):
        """Check for unsaved changes and prompt user. Returns True if OK to proceed."""
        if self.sm_modified or self.anim_modified:
            result = messagebox.askyesnocancel(
                "Unsaved Changes",
                f"Save changes to '{self.current_entity_type}'?"
            )
            if result is None:  # Cancel
                return False
            if result:  # Yes
                self.save_current()
        return True

    # ============== State Machine Methods ==============

    def load_state_machine(self, filepath):
        try:
            self.states = {}
            self.state_order = []
            current_states = []

            with open(filepath, 'r') as f:
                for line in f:
                    line = line.strip()
                    if not line:
                        continue

                    if line.startswith('*') and line.endswith('*'):
                        state_name = line[1:-1]
                        if state_name not in self.states:
                            self.states[state_name] = AnimState(state_name)
                            self.state_order.append(state_name)
                        current_states = [state_name]
                    elif ':' in line:
                        parts = line.split(':', 1)
                        target = parts[0].strip()
                        cond_str = parts[1].strip() if len(parts) > 1 else ""

                        transition = AnimTransition(target)

                        if cond_str:
                            cond_parts = cond_str.split('&&')
                            for cond in cond_parts:
                                cond = cond.strip()
                                tokens = cond.split()
                                if len(tokens) >= 4:
                                    condition = AnimCondition(
                                        var_type=tokens[0],
                                        var_name=tokens[1],
                                        operator=tokens[2],
                                        value=tokens[3]
                                    )
                                    transition.conditions.append(condition)

                        for state_name in current_states:
                            self.states[state_name].transitions.append(transition)

            self.sm_current_file = filepath
            self.selected_state_name = None
            self.refresh_states()
            self.refresh_transitions()
            self.update_target_combo()
            self.sm_modified = False
            self.update_title()
            self.status_var.set(f"Loaded: {filepath} ({len(self.states)} states)")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to load file:\n{e}")

    def write_state_machine(self, filepath):
        try:
            with open(filepath, 'w') as f:
                for state_name in self.state_order:
                    state = self.states[state_name]
                    f.write(f"*{state.name}*\n")
                    for trans in state.transitions:
                        f.write(f"{trans.to_string()}\n")
                    f.write("\n")
            self.sm_modified = False
            self.update_title()
            self.status_var.set(f"Saved: {filepath}")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to save file:\n{e}")

    def get_sorted_states(self):
        return sorted(self.state_order, key=str.lower)

    def refresh_states(self):
        if not hasattr(self, 'state_listbox'):
            return
        self.state_listbox.delete(0, tk.END)
        for name in self.get_sorted_states():
            self.state_listbox.insert(tk.END, name)
        self.update_target_combo()

    def refresh_transitions(self):
        if not hasattr(self, 'trans_tree'):
            return
        self.trans_tree.delete(*self.trans_tree.get_children())
        state = self.get_selected_state()
        if state:
            sorted_transitions = sorted(state.transitions, key=lambda t: t.target_state.lower())
            for trans in sorted_transitions:
                cond_str = " && ".join(str(c) for c in trans.conditions)
                self.trans_tree.insert("", tk.END, values=(trans.target_state, cond_str))

    def update_target_combo(self):
        if hasattr(self, 'target_combo'):
            self.target_combo['values'] = self.get_sorted_states()

    def on_state_click(self, event):
        self.root.after(10, self._handle_state_selection)

    def _handle_state_selection(self):
        selection = self.state_listbox.curselection()
        if selection:
            sorted_states = self.get_sorted_states()
            new_selection = sorted_states[selection[0]]
            if new_selection != self.selected_state_name:
                self.selected_state_name = new_selection
                self.refresh_transitions()
                self.clear_transition_form()

    def add_state(self):
        dialog = tk.Toplevel(self.root)
        dialog.title("Add State")
        dialog.geometry("300x100")
        dialog.transient(self.root)
        dialog.grab_set()

        ttk.Label(dialog, text="State Name:").pack(pady=5)
        name_entry = ttk.Entry(dialog, width=30)
        name_entry.pack(pady=5)
        name_entry.focus_set()

        def do_add():
            name = name_entry.get().strip()
            if name:
                if name in self.states:
                    messagebox.showwarning("Warning", f"State '{name}' already exists")
                    return
                self.states[name] = AnimState(name)
                self.state_order.append(name)
                self.refresh_states()
                self.sm_modified = True
                self.update_title()
                dialog.destroy()

        name_entry.bind("<Return>", lambda e: do_add())
        ttk.Button(dialog, text="Add", command=do_add).pack(pady=5)

    def delete_state(self):
        if not self.selected_state_name:
            return
        state_name = self.selected_state_name
        if messagebox.askyesno("Confirm", f"Delete state '{state_name}' and all its transitions?"):
            del self.states[state_name]
            self.state_order.remove(state_name)
            self.selected_state_name = None
            self.refresh_states()
            self.refresh_transitions()
            self.sm_modified = True
            self.update_title()

    def rename_state(self):
        if not self.selected_state_name:
            return

        old_name = self.selected_state_name

        dialog = tk.Toplevel(self.root)
        dialog.title("Rename State")
        dialog.geometry("300x100")
        dialog.transient(self.root)
        dialog.grab_set()

        ttk.Label(dialog, text="New Name:").pack(pady=5)
        name_entry = ttk.Entry(dialog, width=30)
        name_entry.insert(0, old_name)
        name_entry.pack(pady=5)
        name_entry.focus_set()
        name_entry.select_range(0, tk.END)

        def do_rename():
            new_name = name_entry.get().strip()
            if new_name and new_name != old_name:
                if new_name in self.states:
                    messagebox.showwarning("Warning", f"State '{new_name}' already exists")
                    return
                state = self.states[old_name]
                state.name = new_name
                del self.states[old_name]
                self.states[new_name] = state

                idx = self.state_order.index(old_name)
                self.state_order[idx] = new_name

                for s in self.states.values():
                    for trans in s.transitions:
                        if trans.target_state == old_name:
                            trans.target_state = new_name

                self.selected_state_name = new_name

                self.refresh_states()
                self.refresh_transitions()
                self.sm_modified = True
                self.update_title()
            dialog.destroy()

        name_entry.bind("<Return>", lambda e: do_rename())
        ttk.Button(dialog, text="Rename", command=do_rename).pack(pady=5)

    def get_selected_state(self):
        if self.selected_state_name and self.selected_state_name in self.states:
            return self.states[self.selected_state_name]
        return None

    def get_selected_transition(self):
        state = self.get_selected_state()
        trans_selection = self.trans_tree.selection()
        if state and trans_selection:
            trans_idx = self.trans_tree.index(trans_selection[0])
            sorted_transitions = sorted(state.transitions, key=lambda t: t.target_state.lower())
            if trans_idx < len(sorted_transitions):
                return sorted_transitions[trans_idx]
        return None

    def edit_transition(self, event=None):
        trans = self.get_selected_transition()
        if not trans:
            return

        self.target_combo.set(trans.target_state)

        self.clear_condition_fields()
        for i, cond in enumerate(trans.conditions[:3]):
            type_combo, name_entry, op_combo, value_entry = self.cond_entries[i]
            type_combo.set(cond.var_type)
            name_entry.insert(0, cond.var_name)
            op_combo.set(cond.operator)
            value_entry.insert(0, cond.value)

    def delete_transition(self):
        state = self.get_selected_state()
        trans = self.get_selected_transition()

        if not state or not trans:
            return

        if messagebox.askyesno("Confirm", "Delete this transition?"):
            state.transitions.remove(trans)
            self.refresh_transitions()
            self.sm_modified = True
            self.update_title()

    def _get_transition_from_form(self):
        """Helper to get transition data from form fields."""
        state = self.get_selected_state()
        if not state:
            messagebox.showwarning("Warning", "Please select a state first")
            return None, None

        target = self.target_combo.get().strip()
        if not target:
            messagebox.showwarning("Warning", "Please select a target state")
            return None, None

        conditions = []
        for type_combo, name_entry, op_combo, value_entry in self.cond_entries:
            var_name = name_entry.get().strip()
            if var_name:
                conditions.append(AnimCondition(
                    var_type=type_combo.get(),
                    var_name=var_name,
                    operator=op_combo.get(),
                    value=value_entry.get().strip() or "true"
                ))

        if not conditions:
            messagebox.showwarning("Warning", "Please add at least one condition")
            return None, None

        return state, AnimTransition(target, conditions)

    def add_transition(self):
        state, new_trans = self._get_transition_from_form()
        if not state or not new_trans:
            return

        state.transitions.append(new_trans)
        self.refresh_transitions()
        self.clear_transition_form()
        self.sm_modified = True
        self.update_title()
        self.status_var.set("Transition added")

    def save_transition(self):
        trans = self.get_selected_transition()
        if not trans:
            messagebox.showwarning("Warning", "Please select a transition to save")
            return

        state, new_trans = self._get_transition_from_form()
        if not state or not new_trans:
            return

        trans.target_state = new_trans.target_state
        trans.conditions = new_trans.conditions

        self.refresh_transitions()
        self.clear_transition_form()
        self.sm_modified = True
        self.update_title()
        self.status_var.set("Transition saved")

    def clear_transition_form(self):
        self.target_combo.set("")
        self.clear_condition_fields()

    def clear_condition_fields(self):
        for type_combo, name_entry, op_combo, value_entry in self.cond_entries:
            type_combo.set("bool")
            name_entry.delete(0, tk.END)
            op_combo.set("=")
            value_entry.delete(0, tk.END)

    # ============== Animations Methods ==============

    def load_animations(self, filepath):
        try:
            self.animations = []
            with open(filepath, 'r') as f:
                for line in f:
                    line = line.strip()
                    if line:
                        anim = AnimationDef.from_line(line)
                        if anim:
                            self.animations.append(anim)

            self.anim_current_file = filepath
            self.selected_anim_index = None
            self.refresh_animations()
            self.anim_modified = False
            self.update_title()
            self.status_var.set(f"Loaded: {filepath} ({len(self.animations)} animations)")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to load file:\n{e}")

    def write_animations(self, filepath):
        try:
            with open(filepath, 'w') as f:
                for anim in self.animations:
                    f.write(anim.to_line() + "\n")
            self.anim_modified = False
            self.update_title()
            self.status_var.set(f"Saved: {filepath}")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to save file:\n{e}")

    def refresh_animations(self):
        if not hasattr(self, 'anim_tree'):
            return
        self.anim_tree.delete(*self.anim_tree.get_children())
        for anim in sorted(self.animations, key=lambda a: a.name.lower()):
            frames = f"{anim.start_frame}-{anim.end_frame}"
            size = f"{anim.frame_width}x{anim.frame_height}"
            pivot = f"{anim.pivot_x},{anim.pivot_y}"
            self.anim_tree.insert("", tk.END, values=(
                anim.name, anim.speed, frames, size, anim.filepath, pivot
            ))

    def get_selected_animation(self):
        selection = self.anim_tree.selection()
        if selection:
            # Find by name since list is sorted
            item = self.anim_tree.item(selection[0])
            name = item['values'][0]
            for anim in self.animations:
                if anim.name == name:
                    return anim
        return None

    def edit_animation(self):
        anim = self.get_selected_animation()
        if not anim:
            return

        self.anim_name_entry.delete(0, tk.END)
        self.anim_name_entry.insert(0, anim.name)
        self.anim_speed_entry.delete(0, tk.END)
        self.anim_speed_entry.insert(0, str(anim.speed))
        self.anim_start_entry.delete(0, tk.END)
        self.anim_start_entry.insert(0, str(anim.start_frame))
        self.anim_end_entry.delete(0, tk.END)
        self.anim_end_entry.insert(0, str(anim.end_frame))
        self.anim_width_entry.delete(0, tk.END)
        self.anim_width_entry.insert(0, str(anim.frame_width))
        self.anim_height_entry.delete(0, tk.END)
        self.anim_height_entry.insert(0, str(anim.frame_height))
        self.anim_filepath_entry.delete(0, tk.END)
        self.anim_filepath_entry.insert(0, anim.filepath)
        self.anim_pivotx_entry.delete(0, tk.END)
        self.anim_pivotx_entry.insert(0, str(anim.pivot_x))
        self.anim_pivoty_entry.delete(0, tk.END)
        self.anim_pivoty_entry.insert(0, str(anim.pivot_y))

    def delete_animation(self):
        anim = self.get_selected_animation()
        if not anim:
            return

        if messagebox.askyesno("Confirm", f"Delete animation '{anim.name}'?"):
            self.animations.remove(anim)
            self.refresh_animations()
            self.anim_modified = True
            self.update_title()

    def _get_animation_from_form(self):
        """Helper to get animation data from form fields."""
        name = self.anim_name_entry.get().strip()
        if not name:
            messagebox.showwarning("Warning", "Please enter an animation name")
            return None

        try:
            return AnimationDef(
                name=name,
                speed=int(self.anim_speed_entry.get() or 100),
                start_frame=int(self.anim_start_entry.get() or 0),
                end_frame=int(self.anim_end_entry.get() or 0),
                frame_width=int(self.anim_width_entry.get() or 32),
                frame_height=int(self.anim_height_entry.get() or 32),
                filepath=self.anim_filepath_entry.get().strip(),
                pivot_x=int(self.anim_pivotx_entry.get() or 0),
                pivot_y=int(self.anim_pivoty_entry.get() or 0)
            )
        except ValueError as e:
            messagebox.showerror("Error", f"Invalid number format: {e}")
            return None

    def add_animation(self):
        anim_data = self._get_animation_from_form()
        if not anim_data:
            return

        # Check for duplicate name
        for anim in self.animations:
            if anim.name == anim_data.name:
                if not messagebox.askyesno("Confirm", f"Animation '{anim_data.name}' already exists. Overwrite?"):
                    return
                self.animations.remove(anim)
                break

        self.animations.append(anim_data)
        self.refresh_animations()
        self.clear_animation_form()
        self.anim_modified = True
        self.update_title()
        self.status_var.set(f"Animation '{anim_data.name}' added")

    def save_animation(self):
        existing = self.get_selected_animation()
        if not existing:
            messagebox.showwarning("Warning", "Please select an animation to save")
            return

        anim_data = self._get_animation_from_form()
        if not anim_data:
            return

        # If name changed, check for duplicates
        if anim_data.name != existing.name:
            for anim in self.animations:
                if anim.name == anim_data.name:
                    messagebox.showwarning("Warning", f"Animation '{anim_data.name}' already exists")
                    return

        # Update existing
        existing.name = anim_data.name
        existing.speed = anim_data.speed
        existing.start_frame = anim_data.start_frame
        existing.end_frame = anim_data.end_frame
        existing.frame_width = anim_data.frame_width
        existing.frame_height = anim_data.frame_height
        existing.filepath = anim_data.filepath
        existing.pivot_x = anim_data.pivot_x
        existing.pivot_y = anim_data.pivot_y

        self.refresh_animations()
        self.clear_animation_form()
        self.anim_modified = True
        self.update_title()
        self.status_var.set(f"Animation '{anim_data.name}' saved")

    def clear_animation_form(self):
        if hasattr(self, 'anim_name_entry'):
            self.anim_name_entry.delete(0, tk.END)
            self.anim_speed_entry.delete(0, tk.END)
            self.anim_start_entry.delete(0, tk.END)
            self.anim_end_entry.delete(0, tk.END)
            self.anim_width_entry.delete(0, tk.END)
            self.anim_height_entry.delete(0, tk.END)
            self.anim_filepath_entry.delete(0, tk.END)
            self.anim_pivotx_entry.delete(0, tk.END)
            self.anim_pivoty_entry.delete(0, tk.END)
        if hasattr(self, 'anim_tree'):
            self.anim_tree.selection_remove(*self.anim_tree.selection())
        self.stop_preview()

    # ============== Animation Preview Methods ==============

    def get_game_root(self):
        """Get the game root directory."""
        return self.game_root

    def resolve_asset_path(self, filepath):
        """Convert a game-root-relative path to an absolute path for loading.
        Also substitutes {1}, {2}, etc. with values from the variant dropdown."""
        # Substitute {1} with selected variant
        if hasattr(self, 'variant_var'):
            variant = self.variant_var.get()
            if variant:
                filepath = filepath.replace("{1}", variant)

        game_root = self.get_game_root()
        if game_root and not os.path.isabs(filepath):
            return os.path.join(game_root, filepath)
        return filepath

    def browse_sprite_file(self):
        """Browse for a sprite image file."""
        # Start from the assets folder if we know the game root
        game_root = self.get_game_root()
        if game_root:
            initial_dir = os.path.join(game_root, "assets")
            if not os.path.exists(initial_dir):
                initial_dir = game_root
        else:
            initial_dir = "."

        filepath = filedialog.askopenfilename(
            title="Select Sprite Image",
            filetypes=[("Image files", "*.png *.jpg *.jpeg *.bmp *.gif"), ("All files", "*.*")],
            initialdir=initial_dir
        )
        if filepath:
            # Make path relative to game root
            if game_root:
                try:
                    filepath = os.path.relpath(filepath, game_root)
                    # Normalize to forward slashes for consistency
                    filepath = filepath.replace("\\", "/")
                except ValueError:
                    pass  # Different drives on Windows, keep absolute
            self.anim_filepath_entry.delete(0, tk.END)
            self.anim_filepath_entry.insert(0, filepath)

    def on_animation_select(self, event=None):
        """Called when an animation is selected in the treeview."""
        anim = self.get_selected_animation()
        if anim:
            self.start_preview(anim)

    def preview_from_form(self):
        """Preview animation using current form values."""
        anim_data = self._get_animation_from_form()
        if anim_data:
            self.start_preview(anim_data)

    def start_preview(self, anim):
        """Start playing an animation preview."""
        self.stop_preview()

        if not HAS_PIL:
            self.preview_info_var.set("PIL not installed")
            return

        # Resolve filepath (game-root-relative to absolute)
        filepath = self.resolve_asset_path(anim.filepath)

        if not os.path.exists(filepath):
            self.preview_info_var.set(f"File not found:\n{anim.filepath}")
            self.preview_canvas.delete("all")
            return

        try:
            # Load the spritesheet
            img = Image.open(filepath)
            img_width, img_height = img.size

            frame_width = anim.frame_width
            frame_height = anim.frame_height

            if frame_width <= 0 or frame_height <= 0:
                self.preview_info_var.set("Invalid frame size")
                return

            # Calculate frames per row
            frames_per_row = img_width // frame_width

            if frames_per_row <= 0:
                self.preview_info_var.set("Frame width > image width")
                return

            # Extract frames
            self.preview_frames = []
            for frame_num in range(anim.start_frame, anim.end_frame + 1):
                col = frame_num % frames_per_row
                row = frame_num // frames_per_row

                x1 = col * frame_width
                y1 = row * frame_height
                x2 = x1 + frame_width
                y2 = y1 + frame_height

                # Check bounds
                if x2 > img_width or y2 > img_height:
                    break

                frame = img.crop((x1, y1, x2, y2))

                # Scale up small frames for visibility (max 2x or fit in 200x200)
                canvas_size = 200
                scale = min(canvas_size / frame_width, canvas_size / frame_height, 4)
                if scale > 1:
                    new_size = (int(frame_width * scale), int(frame_height * scale))
                    frame = frame.resize(new_size, Image.NEAREST)

                photo = ImageTk.PhotoImage(frame)
                self.preview_frames.append(photo)

            if not self.preview_frames:
                self.preview_info_var.set("No frames extracted")
                return

            # Start animation
            self.preview_frame_index = 0
            self.preview_speed = max(anim.speed, 10)  # Minimum 10ms
            num_frames = anim.end_frame - anim.start_frame + 1
            self.preview_info_var.set(f"{anim.name}: {num_frames} frames @ {anim.speed}ms")
            self.animate_preview()

        except Exception as e:
            self.preview_info_var.set(f"Error: {str(e)[:30]}")
            self.preview_canvas.delete("all")

    def stop_preview(self):
        """Stop the animation preview."""
        if self.preview_after_id:
            self.root.after_cancel(self.preview_after_id)
            self.preview_after_id = None
        self.preview_frames = []
        self.preview_frame_index = 0
        if hasattr(self, 'preview_canvas'):
            self.preview_canvas.delete("all")
        if hasattr(self, 'preview_info_var'):
            self.preview_info_var.set("Select an animation to preview")

    def animate_preview(self):
        """Animation loop for preview."""
        if not self.preview_frames:
            return

        # Clear and draw current frame
        self.preview_canvas.delete("all")
        frame = self.preview_frames[self.preview_frame_index]

        # Center the frame in the canvas
        canvas_width = self.preview_canvas.winfo_width()
        canvas_height = self.preview_canvas.winfo_height()
        x = canvas_width // 2
        y = canvas_height // 2

        self.preview_canvas.create_image(x, y, image=frame, anchor=tk.CENTER)

        # Keep reference to prevent garbage collection
        self.preview_photo = frame

        # Advance to next frame
        self.preview_frame_index = (self.preview_frame_index + 1) % len(self.preview_frames)

        # Schedule next frame
        self.preview_after_id = self.root.after(self.preview_speed, self.animate_preview)

    # ============== Common Methods ==============

    def update_title(self):
        title = "Animator Editor"
        if self.game_root:
            project_name = os.path.basename(self.game_root)
            title += f" - {project_name}"
            if self.current_entity_type:
                modified = "*" if (self.sm_modified or self.anim_modified) else ""
                title += f" / {self.current_entity_type}{modified}"
        self.root.title(title)

    def on_close(self):
        if not self.check_unsaved_changes():
            return
        self.root.destroy()


def main():
    root = tk.Tk()
    app = AnimatorEditor(root)
    root.protocol("WM_DELETE_WINDOW", app.on_close)
    root.mainloop()


if __name__ == "__main__":
    main()
