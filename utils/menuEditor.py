"""
Menu Editor - Visual designer for game menu layouts
Generates C++ code snippets and .menu data files for MenuScreen setup

This tool helps you:
1. Position buttons, text, and images visually
2. Configure button navigation (up/down/left/right)
3. Preview actual sprites from your game project
4. Create enter/exit animations with keyframes
5. Export C++ code or .menu data files
"""

import tkinter as tk
from tkinter import ttk, filedialog, messagebox, simpledialog, colorchooser
import json
import os

# Try to import PIL for image preview
try:
    from PIL import Image, ImageTk
    HAS_PIL = True
except ImportError:
    HAS_PIL = False


class MenuTemplate:
    """Template for repeating elements based on dynamic data"""
    def __init__(self, name=""):
        self.name = name
        self.element_type = "button"  # "button", "text", "image"
        self.data_source = ""  # Name of the data source (e.g., "spells", "inventory")
        self.filter = ""  # Optional filter condition

        # Layout settings
        self.layout = "grid"  # "grid", "horizontal", "vertical"
        self.columns = 3
        self.start_x = 0
        self.start_y = 0
        self.spacing_x = 240
        self.spacing_y = 320

        # Element properties (can use {variable} placeholders)
        self.text = "{name}"
        self.filepath_pattern = ""
        self.button_id = 0
        self.width = 150
        self.height = 40
        self.scale_x = 1.0
        self.scale_y = 1.0

    def to_dict(self):
        return {
            "name": self.name,
            "type": self.element_type,
            "data_source": self.data_source,
            "filter": self.filter,
            "layout": self.layout,
            "columns": self.columns,
            "start_x": self.start_x,
            "start_y": self.start_y,
            "spacing_x": self.spacing_x,
            "spacing_y": self.spacing_y,
            "text": self.text,
            "filepath_pattern": self.filepath_pattern,
            "button_id": self.button_id,
            "width": self.width,
            "height": self.height,
            "scale_x": self.scale_x,
            "scale_y": self.scale_y
        }

    @staticmethod
    def from_dict(data):
        t = MenuTemplate(data.get("name", ""))
        t.element_type = data.get("type", "button")
        t.data_source = data.get("data_source", "")
        t.filter = data.get("filter", "")
        t.layout = data.get("layout", "grid")
        t.columns = data.get("columns", 3)
        t.start_x = data.get("start_x", 0)
        t.start_y = data.get("start_y", 0)
        t.spacing_x = data.get("spacing_x", 240)
        t.spacing_y = data.get("spacing_y", 320)
        t.text = data.get("text", "{name}")
        t.filepath_pattern = data.get("filepath_pattern", "")
        t.button_id = data.get("button_id", 0)
        t.width = data.get("width", 150)
        t.height = data.get("height", 40)
        t.scale_x = data.get("scale_x", 1.0)
        t.scale_y = data.get("scale_y", 1.0)
        return t


class MenuSlot:
    """Slot for C++ to inject dynamic content"""
    def __init__(self, name=""):
        self.name = name
        self.x = 0
        self.y = 0
        self.layout = "vertical"  # "vertical", "horizontal", "grid"
        self.columns = 1
        self.spacing_x = 0
        self.spacing_y = 120

    def to_dict(self):
        return {
            "name": self.name,
            "x": self.x,
            "y": self.y,
            "layout": self.layout,
            "columns": self.columns,
            "spacing_x": self.spacing_x,
            "spacing_y": self.spacing_y
        }

    @staticmethod
    def from_dict(data):
        s = MenuSlot(data.get("name", ""))
        s.x = data.get("x", 0)
        s.y = data.get("y", 0)
        s.layout = data.get("layout", "vertical")
        s.columns = data.get("columns", 1)
        s.spacing_x = data.get("spacing_x", 0)
        s.spacing_y = data.get("spacing_y", 120)
        return s


class AnimationKeyframe:
    """Represents a single keyframe in a menu animation"""
    def __init__(self, duration=500):
        self.duration = duration
        self.target_x = 0
        self.target_y = 0
        self.target_alpha = 255
        self.use_position = True
        self.use_alpha = False

    def to_dict(self):
        return {
            "duration": self.duration,
            "target_x": self.target_x,
            "target_y": self.target_y,
            "target_alpha": self.target_alpha,
            "use_position": self.use_position,
            "use_alpha": self.use_alpha
        }

    @staticmethod
    def from_dict(data):
        kf = AnimationKeyframe(data.get("duration", 500))
        kf.target_x = data.get("target_x", 0)
        kf.target_y = data.get("target_y", 0)
        kf.target_alpha = data.get("target_alpha", 255)
        kf.use_position = data.get("use_position", True)
        kf.use_alpha = data.get("use_alpha", False)
        return kf


class MenuElement:
    def __init__(self, elem_type, name, x, y, width=100, height=30):
        self.elem_type = elem_type  # "button", "text", "image"
        self.name = name
        self.x = x
        self.y = y
        self.width = width
        self.height = height
        self.text = name
        self.filepath = ""
        self.font_size = 24
        self.color = [255, 255, 255, 255]
        self.scale = [1.0, 1.0]
        self.button_id = 0
        self.centered = False
        # Navigation links
        self.nav_up = ""
        self.nav_down = ""
        self.nav_left = ""
        self.nav_right = ""
        # Animations
        self.enter_keyframes = []
        self.exit_keyframes = []

    def to_dict(self):
        return {
            "type": self.elem_type,
            "name": self.name,
            "x": self.x,
            "y": self.y,
            "width": self.width,
            "height": self.height,
            "text": self.text,
            "filepath": self.filepath,
            "font_size": self.font_size,
            "color": self.color,
            "scale": self.scale,
            "button_id": self.button_id,
            "centered": self.centered,
            "nav_up": self.nav_up,
            "nav_down": self.nav_down,
            "nav_left": self.nav_left,
            "nav_right": self.nav_right,
            "enter_keyframes": [kf.to_dict() for kf in self.enter_keyframes],
            "exit_keyframes": [kf.to_dict() for kf in self.exit_keyframes]
        }

    @staticmethod
    def from_dict(data):
        elem = MenuElement(data["type"], data["name"], data["x"], data["y"],
                          data.get("width", 100), data.get("height", 30))
        elem.text = data.get("text", elem.name)
        elem.filepath = data.get("filepath", "")
        elem.font_size = data.get("font_size", 24)
        elem.color = list(data.get("color", [255, 255, 255, 255]))
        elem.scale = list(data.get("scale", [1.0, 1.0]))
        elem.button_id = data.get("button_id", 0)
        elem.centered = data.get("centered", False)
        elem.nav_up = data.get("nav_up", "")
        elem.nav_down = data.get("nav_down", "")
        elem.nav_left = data.get("nav_left", "")
        elem.nav_right = data.get("nav_right", "")
        elem.enter_keyframes = [AnimationKeyframe.from_dict(kf) for kf in data.get("enter_keyframes", [])]
        elem.exit_keyframes = [AnimationKeyframe.from_dict(kf) for kf in data.get("exit_keyframes", [])]
        return elem


class MenuEditor:
    def __init__(self, root):
        self.root = root
        self.root.title("Menu Editor")
        self.root.geometry("1400x900")

        # Project settings
        self.game_root = None
        self.current_file = None
        self.menu_name = "MyMenu"
        self.elements = []
        self.selected_element = None
        self.drag_data = {"x": 0, "y": 0, "item": None}
        self.modified = False

        # Menu screens list
        self.menu_screens = []  # List of menu names
        self.current_menu_index = -1

        # Templates and slots for dynamic content
        self.templates = []
        self.slots = []
        self.selected_template = None
        self.selected_slot = None

        # Menu properties
        self.can_escape_from = True
        self.is_dynamic = False
        self.use_mouse = False
        self.remember_last_button = False

        # Canvas dimensions (typical game resolution)
        self.canvas_width = 1280
        self.canvas_height = 720
        self.canvas_scale = 0.5  # Display scale

        # Image cache for preview
        self.image_cache = {}
        self.show_image_preview = HAS_PIL

        self.create_menu()
        self.create_main_area()
        self.create_status_bar()

    def create_menu(self):
        menubar = tk.Menu(self.root)
        self.root.config(menu=menubar)

        file_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="File", menu=file_menu)
        file_menu.add_command(label="Open Project...", command=self.open_project, accelerator="Ctrl+Shift+O")
        file_menu.add_separator()
        file_menu.add_command(label="New Menu", command=self.new_menu, accelerator="Ctrl+N")
        file_menu.add_command(label="Open Menu...", command=self.open_file, accelerator="Ctrl+O")
        file_menu.add_command(label="Save Menu", command=self.save_file, accelerator="Ctrl+S")
        file_menu.add_command(label="Save Menu As...", command=self.save_file_as)
        file_menu.add_separator()
        file_menu.add_command(label="Export C++ Code...", command=self.export_cpp)
        file_menu.add_command(label="Export .menu File...", command=self.export_menu_data)
        file_menu.add_separator()
        file_menu.add_command(label="Exit", command=self.on_close)

        edit_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="Edit", menu=edit_menu)
        edit_menu.add_command(label="Menu Settings...", command=self.edit_menu_settings)
        edit_menu.add_command(label="Canvas Size...", command=self.edit_canvas_size)
        edit_menu.add_separator()
        edit_menu.add_command(label="Delete Selected", command=self.delete_selected, accelerator="Del")
        edit_menu.add_command(label="Duplicate Selected", command=self.duplicate_selected, accelerator="Ctrl+D")
        edit_menu.add_separator()
        edit_menu.add_command(label="Auto-assign Navigation", command=self.auto_assign_navigation)

        view_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="View", menu=view_menu)

        self.show_preview_var = tk.BooleanVar(value=HAS_PIL)
        view_menu.add_checkbutton(label="Show Image Preview", variable=self.show_preview_var,
                                   command=self.toggle_preview)

        view_menu.add_separator()
        view_menu.add_command(label="Zoom In", command=lambda: self.set_zoom(self.canvas_scale + 0.1))
        view_menu.add_command(label="Zoom Out", command=lambda: self.set_zoom(self.canvas_scale - 0.1))
        view_menu.add_command(label="Reset Zoom", command=lambda: self.set_zoom(0.5))

        self.root.bind("<Control-Shift-O>", lambda e: self.open_project())
        self.root.bind("<Control-n>", lambda e: self.new_menu())
        self.root.bind("<Control-o>", lambda e: self.open_file())
        self.root.bind("<Control-s>", lambda e: self.save_file())
        self.root.bind("<Control-d>", lambda e: self.duplicate_selected())
        self.root.bind("<Delete>", lambda e: self.delete_selected())

    def create_main_area(self):
        main_paned = ttk.PanedWindow(self.root, orient=tk.HORIZONTAL)
        main_paned.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        # Left panel - Menu screens list
        left_frame = ttk.Frame(main_paned, width=180)
        main_paned.add(left_frame, weight=0)

        screens_frame = ttk.LabelFrame(left_frame, text="Menu Screens")
        screens_frame.pack(fill=tk.BOTH, expand=True, padx=2, pady=2)

        # Screens listbox with scrollbar
        screens_scroll = ttk.Scrollbar(screens_frame)
        screens_scroll.pack(side=tk.RIGHT, fill=tk.Y)

        self.screens_listbox = tk.Listbox(screens_frame, yscrollcommand=screens_scroll.set,
                                           selectmode=tk.SINGLE, width=20)
        self.screens_listbox.pack(fill=tk.BOTH, expand=True, padx=2, pady=2)
        screens_scroll.config(command=self.screens_listbox.yview)
        self.screens_listbox.bind("<<ListboxSelect>>", self.on_screen_select)

        # Screen buttons
        btn_frame = ttk.Frame(screens_frame)
        btn_frame.pack(fill=tk.X, padx=2, pady=2)
        ttk.Button(btn_frame, text="Add", width=6, command=self.add_menu_screen).pack(side=tk.LEFT, padx=1)
        ttk.Button(btn_frame, text="Del", width=6, command=self.delete_menu_screen).pack(side=tk.LEFT, padx=1)
        ttk.Button(btn_frame, text="Reload", width=6, command=self.reload_screens_list).pack(side=tk.LEFT, padx=1)

        # Center panel - Canvas
        center_frame = ttk.Frame(main_paned)
        main_paned.add(center_frame, weight=3)

        # Toolbar
        toolbar = ttk.Frame(center_frame)
        toolbar.pack(fill=tk.X, pady=2)

        ttk.Button(toolbar, text="+ Button", command=lambda: self.add_element("button")).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="+ Text", command=lambda: self.add_element("text")).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="+ Image", command=lambda: self.add_element("image")).pack(side=tk.LEFT, padx=2)

        ttk.Separator(toolbar, orient=tk.VERTICAL).pack(side=tk.LEFT, fill=tk.Y, padx=5)

        ttk.Label(toolbar, text="Scale:").pack(side=tk.LEFT, padx=2)
        self.scale_combo = ttk.Combobox(toolbar, values=["25%", "50%", "75%", "100%"], width=6, state="readonly")
        self.scale_combo.set("50%")
        self.scale_combo.pack(side=tk.LEFT, padx=2)
        self.scale_combo.bind("<<ComboboxSelected>>", self.on_scale_change)

        # Canvas with scrollbars
        canvas_frame = ttk.Frame(center_frame)
        canvas_frame.pack(fill=tk.BOTH, expand=True)

        display_w = int(self.canvas_width * self.canvas_scale)
        display_h = int(self.canvas_height * self.canvas_scale)

        self.canvas = tk.Canvas(canvas_frame, bg="#333333", width=display_w, height=display_h)
        h_scroll = ttk.Scrollbar(canvas_frame, orient=tk.HORIZONTAL, command=self.canvas.xview)
        v_scroll = ttk.Scrollbar(canvas_frame, orient=tk.VERTICAL, command=self.canvas.yview)
        self.canvas.configure(xscrollcommand=h_scroll.set, yscrollcommand=v_scroll.set)

        self.canvas.grid(row=0, column=0, sticky="nsew")
        v_scroll.grid(row=0, column=1, sticky="ns")
        h_scroll.grid(row=1, column=0, sticky="ew")
        canvas_frame.rowconfigure(0, weight=1)
        canvas_frame.columnconfigure(0, weight=1)

        self.canvas.configure(scrollregion=(0, 0, display_w, display_h))

        self.canvas.bind("<Button-1>", self.on_canvas_click)
        self.canvas.bind("<B1-Motion>", self.on_canvas_drag)
        self.canvas.bind("<ButtonRelease-1>", self.on_canvas_release)

        # Right panel - Elements and Properties
        right_frame = ttk.Frame(main_paned, width=320)
        main_paned.add(right_frame, weight=1)

        # Elements list
        list_frame = ttk.LabelFrame(right_frame, text="Elements")
        list_frame.pack(fill=tk.BOTH, expand=True)

        elem_scroll = ttk.Scrollbar(list_frame)
        elem_scroll.pack(side=tk.RIGHT, fill=tk.Y)

        self.elem_listbox = tk.Listbox(list_frame, selectmode=tk.SINGLE, yscrollcommand=elem_scroll.set)
        self.elem_listbox.pack(fill=tk.BOTH, expand=True, padx=2, pady=2)
        elem_scroll.config(command=self.elem_listbox.yview)
        self.elem_listbox.bind("<<ListboxSelect>>", self.on_list_select)

        # Properties notebook
        prop_notebook = ttk.Notebook(right_frame)
        prop_notebook.pack(fill=tk.X, pady=5)

        # Basic properties tab
        basic_frame = ttk.Frame(prop_notebook)
        prop_notebook.add(basic_frame, text="Properties")

        # Property entries
        props = [
            ("Name:", "name_entry"),
            ("Text:", "text_entry"),
            ("X:", "x_entry"),
            ("Y:", "y_entry"),
            ("Width:", "width_entry"),
            ("Height:", "height_entry"),
            ("Filepath:", "filepath_entry"),
            ("Button ID:", "btnid_entry"),
        ]

        for i, (label, attr) in enumerate(props):
            ttk.Label(basic_frame, text=label).grid(row=i, column=0, sticky=tk.W, padx=2, pady=1)
            entry = ttk.Entry(basic_frame, width=20)
            entry.grid(row=i, column=1, sticky=tk.W+tk.E, padx=2, pady=1)
            setattr(self, attr, entry)

        # Browse button for filepath
        ttk.Button(basic_frame, text="...", width=3, command=self.browse_filepath).grid(
            row=6, column=2, padx=2, pady=1)

        basic_frame.columnconfigure(1, weight=1)

        # Scale and color row
        row = len(props)
        ttk.Label(basic_frame, text="Scale X:").grid(row=row, column=0, sticky=tk.W, padx=2, pady=1)
        self.scale_x_entry = ttk.Entry(basic_frame, width=8)
        self.scale_x_entry.grid(row=row, column=1, sticky=tk.W, padx=2, pady=1)

        row += 1
        ttk.Label(basic_frame, text="Scale Y:").grid(row=row, column=0, sticky=tk.W, padx=2, pady=1)
        self.scale_y_entry = ttk.Entry(basic_frame, width=8)
        self.scale_y_entry.grid(row=row, column=1, sticky=tk.W, padx=2, pady=1)

        row += 1
        ttk.Label(basic_frame, text="Color:").grid(row=row, column=0, sticky=tk.W, padx=2, pady=1)
        color_frame = ttk.Frame(basic_frame)
        color_frame.grid(row=row, column=1, sticky=tk.W, padx=2, pady=1)

        self.color_preview = tk.Label(color_frame, text="    ", bg="#ffffff", relief=tk.SUNKEN)
        self.color_preview.pack(side=tk.LEFT)
        ttk.Button(color_frame, text="Pick", command=self.pick_color).pack(side=tk.LEFT, padx=2)

        row += 1
        self.centered_var = tk.BooleanVar()
        ttk.Checkbutton(basic_frame, text="Centered", variable=self.centered_var).grid(
            row=row, column=0, columnspan=2, sticky=tk.W, padx=2, pady=1)

        row += 1
        ttk.Button(basic_frame, text="Apply Changes", command=self.apply_properties).grid(
            row=row, column=0, columnspan=3, pady=5)

        # Navigation tab
        nav_frame = ttk.Frame(prop_notebook)
        prop_notebook.add(nav_frame, text="Navigation")

        nav_props = [("Up:", "nav_up_combo"), ("Down:", "nav_down_combo"),
                     ("Left:", "nav_left_combo"), ("Right:", "nav_right_combo")]

        for i, (label, attr) in enumerate(nav_props):
            ttk.Label(nav_frame, text=label).grid(row=i, column=0, sticky=tk.W, padx=2, pady=1)
            combo = ttk.Combobox(nav_frame, width=18)
            combo.grid(row=i, column=1, sticky=tk.W+tk.E, padx=2, pady=1)
            setattr(self, attr, combo)

        nav_frame.columnconfigure(1, weight=1)

        ttk.Button(nav_frame, text="Apply Navigation", command=self.apply_navigation).grid(
            row=len(nav_props), column=0, columnspan=2, pady=5)

        # Animation tab
        anim_frame = ttk.Frame(prop_notebook)
        prop_notebook.add(anim_frame, text="Animation")

        ttk.Label(anim_frame, text="Enter Animation:").grid(row=0, column=0, columnspan=2, sticky=tk.W, padx=2, pady=2)

        self.enter_anim_list = tk.Listbox(anim_frame, height=4, width=30)
        self.enter_anim_list.grid(row=1, column=0, columnspan=2, padx=2, pady=2, sticky=tk.W+tk.E)

        enter_btn_frame = ttk.Frame(anim_frame)
        enter_btn_frame.grid(row=2, column=0, columnspan=2, pady=2)
        ttk.Button(enter_btn_frame, text="Add", command=lambda: self.add_keyframe("enter")).pack(side=tk.LEFT, padx=2)
        ttk.Button(enter_btn_frame, text="Edit", command=lambda: self.edit_keyframe("enter")).pack(side=tk.LEFT, padx=2)
        ttk.Button(enter_btn_frame, text="Del", command=lambda: self.delete_keyframe("enter")).pack(side=tk.LEFT, padx=2)

        ttk.Label(anim_frame, text="Exit Animation:").grid(row=3, column=0, columnspan=2, sticky=tk.W, padx=2, pady=2)

        self.exit_anim_list = tk.Listbox(anim_frame, height=4, width=30)
        self.exit_anim_list.grid(row=4, column=0, columnspan=2, padx=2, pady=2, sticky=tk.W+tk.E)

        exit_btn_frame = ttk.Frame(anim_frame)
        exit_btn_frame.grid(row=5, column=0, columnspan=2, pady=2)
        ttk.Button(exit_btn_frame, text="Add", command=lambda: self.add_keyframe("exit")).pack(side=tk.LEFT, padx=2)
        ttk.Button(exit_btn_frame, text="Edit", command=lambda: self.edit_keyframe("exit")).pack(side=tk.LEFT, padx=2)
        ttk.Button(exit_btn_frame, text="Del", command=lambda: self.delete_keyframe("exit")).pack(side=tk.LEFT, padx=2)

        anim_frame.columnconfigure(0, weight=1)

        # Dynamic Content tab (Templates and Slots)
        dynamic_frame = ttk.Frame(prop_notebook)
        prop_notebook.add(dynamic_frame, text="Dynamic")

        # Templates section
        ttk.Label(dynamic_frame, text="Templates:").grid(row=0, column=0, columnspan=2, sticky=tk.W, padx=2, pady=2)

        self.templates_list = tk.Listbox(dynamic_frame, height=4, width=30)
        self.templates_list.grid(row=1, column=0, columnspan=2, padx=2, pady=2, sticky=tk.W+tk.E)
        self.templates_list.bind("<<ListboxSelect>>", self.on_template_select)

        tmpl_btn_frame = ttk.Frame(dynamic_frame)
        tmpl_btn_frame.grid(row=2, column=0, columnspan=2, pady=2)
        ttk.Button(tmpl_btn_frame, text="Add", command=self.add_template).pack(side=tk.LEFT, padx=2)
        ttk.Button(tmpl_btn_frame, text="Edit", command=self.edit_template).pack(side=tk.LEFT, padx=2)
        ttk.Button(tmpl_btn_frame, text="Del", command=self.delete_template).pack(side=tk.LEFT, padx=2)

        # Slots section
        ttk.Label(dynamic_frame, text="Slots:").grid(row=3, column=0, columnspan=2, sticky=tk.W, padx=2, pady=2)

        self.slots_list = tk.Listbox(dynamic_frame, height=4, width=30)
        self.slots_list.grid(row=4, column=0, columnspan=2, padx=2, pady=2, sticky=tk.W+tk.E)
        self.slots_list.bind("<<ListboxSelect>>", self.on_slot_select)

        slot_btn_frame = ttk.Frame(dynamic_frame)
        slot_btn_frame.grid(row=5, column=0, columnspan=2, pady=2)
        ttk.Button(slot_btn_frame, text="Add", command=self.add_slot).pack(side=tk.LEFT, padx=2)
        ttk.Button(slot_btn_frame, text="Edit", command=self.edit_slot).pack(side=tk.LEFT, padx=2)
        ttk.Button(slot_btn_frame, text="Del", command=self.delete_slot).pack(side=tk.LEFT, padx=2)

        dynamic_frame.columnconfigure(0, weight=1)

    def create_status_bar(self):
        status_frame = ttk.Frame(self.root)
        status_frame.pack(side=tk.BOTTOM, fill=tk.X)

        self.status_var = tk.StringVar(value="Ready - Open a project to begin")
        status_bar = ttk.Label(status_frame, textvariable=self.status_var, relief=tk.SUNKEN, anchor=tk.W)
        status_bar.pack(side=tk.LEFT, fill=tk.X, expand=True)

        self.project_var = tk.StringVar(value="No project")
        project_label = ttk.Label(status_frame, textvariable=self.project_var, relief=tk.SUNKEN, width=40)
        project_label.pack(side=tk.RIGHT)

    def open_project(self):
        """Open a game project folder"""
        folder = filedialog.askdirectory(title="Select Game Project Folder")
        if folder:
            self.game_root = folder
            self.project_var.set(f"Project: {os.path.basename(folder)}")
            self.image_cache.clear()
            self.load_menu_screens_list()
            self.status_var.set(f"Opened project: {folder}")

    def load_menu_screens_list(self):
        """Load menu screens from the project's menuScreens.list file"""
        self.menu_screens = []
        self.screens_listbox.delete(0, tk.END)

        if not self.game_root:
            return

        # Try new lists folder first, then old config location
        list_paths = [
            os.path.join(self.game_root, "data", "lists", "menuScreens.list"),
            os.path.join(self.game_root, "data", "config", "menuScreens.list")
        ]

        for path in list_paths:
            if os.path.exists(path):
                try:
                    with open(path, 'r', encoding='utf-8') as f:
                        for line in f:
                            line = line.strip()
                            if line:
                                self.menu_screens.append(line)
                                self.screens_listbox.insert(tk.END, line)
                    self.status_var.set(f"Loaded {len(self.menu_screens)} menu screens")
                    return
                except Exception as e:
                    self.status_var.set(f"Error loading menu list: {e}")
                    return

        self.status_var.set("No menuScreens.list found")

    def reload_screens_list(self):
        """Reload the menu screens list from file"""
        self.load_menu_screens_list()

    def on_screen_select(self, event):
        """Handle menu screen selection"""
        selection = self.screens_listbox.curselection()
        if selection:
            idx = selection[0]
            screen_name = self.menu_screens[idx]
            self.current_menu_index = idx

            # Check if there's a .menu file for this screen
            if self.game_root:
                menu_paths = [
                    os.path.join(self.game_root, "data", "menus", f"{screen_name}.menu"),
                    os.path.join(self.game_root, "data", "config", f"{screen_name}.menu")
                ]

                for path in menu_paths:
                    if os.path.exists(path):
                        self.load_file(path)
                        return

            # No file found, create new menu with this name
            self.menu_name = screen_name
            self.elements = []
            self.selected_element = None
            self.current_file = None
            self.refresh_canvas()
            self.refresh_list()
            self.update_title()

    def add_menu_screen(self):
        """Add a new menu screen to the list"""
        name = simpledialog.askstring("Add Menu Screen", "Enter menu screen name:")
        if name:
            if name not in self.menu_screens:
                self.menu_screens.append(name)
                self.screens_listbox.insert(tk.END, name)
                self.modified = True
                self.update_title()

    def delete_menu_screen(self):
        """Delete selected menu screen from the list"""
        selection = self.screens_listbox.curselection()
        if selection:
            idx = selection[0]
            if messagebox.askyesno("Confirm", f"Delete menu screen '{self.menu_screens[idx]}'?"):
                del self.menu_screens[idx]
                self.screens_listbox.delete(idx)
                self.modified = True
                self.update_title()

    def toggle_preview(self):
        """Toggle image preview display"""
        self.show_image_preview = self.show_preview_var.get() and HAS_PIL
        self.refresh_canvas()

    def set_zoom(self, scale):
        """Set canvas zoom level"""
        self.canvas_scale = max(0.25, min(1.0, scale))
        self.scale_combo.set(f"{int(self.canvas_scale * 100)}%")

        display_w = int(self.canvas_width * self.canvas_scale)
        display_h = int(self.canvas_height * self.canvas_scale)
        self.canvas.configure(scrollregion=(0, 0, display_w, display_h))
        self.refresh_canvas()

    def on_scale_change(self, event):
        """Handle scale combobox change"""
        scale_str = self.scale_combo.get()
        scale = int(scale_str.replace("%", "")) / 100.0
        self.set_zoom(scale)

    def get_cached_image(self, filepath, width, height):
        """Get an image from cache or load it"""
        if not HAS_PIL or not filepath:
            return None

        # Build full path if relative
        full_path = filepath
        if self.game_root and not os.path.isabs(filepath):
            full_path = os.path.join(self.game_root, filepath)

        cache_key = f"{full_path}_{width}_{height}_{self.canvas_scale}"

        if cache_key in self.image_cache:
            return self.image_cache[cache_key]

        try:
            if os.path.exists(full_path):
                img = Image.open(full_path)

                # Scale to display size
                display_w = int(width * self.canvas_scale)
                display_h = int(height * self.canvas_scale)

                if display_w > 0 and display_h > 0:
                    img = img.resize((display_w, display_h), Image.Resampling.LANCZOS)
                    photo = ImageTk.PhotoImage(img)
                    self.image_cache[cache_key] = photo
                    return photo
        except Exception as e:
            print(f"Error loading image {full_path}: {e}")

        return None

    def refresh_canvas(self):
        self.canvas.delete("all")

        scale = self.canvas_scale
        display_w = int(self.canvas_width * scale)
        display_h = int(self.canvas_height * scale)

        # Draw background
        self.canvas.create_rectangle(0, 0, display_w, display_h, fill="#222222", outline="")

        # Draw grid
        grid_size = 50
        for x in range(0, self.canvas_width, grid_size):
            sx = int(x * scale)
            self.canvas.create_line(sx, 0, sx, display_h, fill="#333333")
        for y in range(0, self.canvas_height, grid_size):
            sy = int(y * scale)
            self.canvas.create_line(0, sy, display_w, sy, fill="#333333")

        # Draw center guides
        cx = int(self.canvas_width / 2 * scale)
        cy = int(self.canvas_height / 2 * scale)
        self.canvas.create_line(cx, 0, cx, display_h, fill="#444444", dash=(4, 4))
        self.canvas.create_line(0, cy, display_w, cy, fill="#444444", dash=(4, 4))

        # Draw elements
        for elem in self.elements:
            x1 = int(elem.x * scale)
            y1 = int(elem.y * scale)
            w = int(elem.width * elem.scale[0] * scale)
            h = int(elem.height * elem.scale[1] * scale)
            x2 = x1 + w
            y2 = y1 + h

            # Try to draw image preview
            image_drawn = False
            if self.show_image_preview and elem.filepath:
                img = self.get_cached_image(elem.filepath,
                                            int(elem.width * elem.scale[0]),
                                            int(elem.height * elem.scale[1]))
                if img:
                    self.canvas.create_image(x1, y1, image=img, anchor=tk.NW, tags=f"elem_{elem.name}")
                    image_drawn = True

            # Draw colored rectangle if no image or for selection highlight
            if not image_drawn:
                fill = "#4488ff" if elem.elem_type == "button" else "#44ff44" if elem.elem_type == "text" else "#ff8844"
                self.canvas.create_rectangle(x1, y1, x2, y2, fill=fill, outline="white",
                                            tags=f"elem_{elem.name}")

            # Draw selection highlight
            if elem == self.selected_element:
                self.canvas.create_rectangle(x1-2, y1-2, x2+2, y2+2, outline="yellow", width=2,
                                            tags=f"select_{elem.name}")

            # Draw text label
            label = elem.text[:20] if elem.text else elem.name[:20]
            text_y = y1 + h // 2 if not image_drawn else y2 + 10
            self.canvas.create_text(x1 + w // 2, text_y, text=label, fill="white",
                                   tags=f"label_{elem.name}")

    def refresh_list(self):
        self.elem_listbox.delete(0, tk.END)
        for elem in self.elements:
            prefix = "[B]" if elem.elem_type == "button" else "[T]" if elem.elem_type == "text" else "[I]"
            self.elem_listbox.insert(tk.END, f"{prefix} {elem.name}")

        self.update_nav_combos()

    def update_nav_combos(self):
        button_names = [""] + [e.name for e in self.elements if e.elem_type == "button"]
        for combo in [self.nav_up_combo, self.nav_down_combo, self.nav_left_combo, self.nav_right_combo]:
            combo['values'] = button_names

    def add_element(self, elem_type):
        name = simpledialog.askstring("Add Element", f"Enter {elem_type} name:")
        if name:
            # Check for duplicate names
            if any(e.name == name for e in self.elements):
                messagebox.showwarning("Warning", f"Element '{name}' already exists")
                return

            elem = MenuElement(elem_type, name, 100, 100)
            if elem_type == "button":
                elem.width = 150
                elem.height = 40
                elem.button_id = len([e for e in self.elements if e.elem_type == "button"])
            elif elem_type == "text":
                elem.width = 200
                elem.height = 30
            else:  # image
                elem.width = 100
                elem.height = 100

            self.elements.append(elem)
            self.refresh_canvas()
            self.refresh_list()
            self.modified = True
            self.update_title()

    def duplicate_selected(self):
        """Duplicate the selected element"""
        if self.selected_element:
            # Create a copy
            new_elem = MenuElement.from_dict(self.selected_element.to_dict())
            new_elem.name = f"{self.selected_element.name}_copy"
            new_elem.x += 20
            new_elem.y += 20

            # Ensure unique name
            base_name = new_elem.name
            counter = 1
            while any(e.name == new_elem.name for e in self.elements):
                new_elem.name = f"{base_name}_{counter}"
                counter += 1

            self.elements.append(new_elem)
            self.selected_element = new_elem
            self.refresh_canvas()
            self.refresh_list()
            self.modified = True
            self.update_title()

    def on_canvas_click(self, event):
        x = self.canvas.canvasx(event.x) / self.canvas_scale
        y = self.canvas.canvasy(event.y) / self.canvas_scale

        # Find clicked element
        self.selected_element = None
        for elem in reversed(self.elements):  # Check top elements first
            ex = elem.x
            ey = elem.y
            ew = elem.width * elem.scale[0]
            eh = elem.height * elem.scale[1]

            if ex <= x <= ex + ew and ey <= y <= ey + eh:
                self.selected_element = elem
                self.drag_data["x"] = x - elem.x
                self.drag_data["y"] = y - elem.y
                break

        self.refresh_canvas()
        self.show_properties()

        # Select in listbox
        if self.selected_element:
            for i, elem in enumerate(self.elements):
                if elem == self.selected_element:
                    self.elem_listbox.selection_clear(0, tk.END)
                    self.elem_listbox.selection_set(i)
                    break

    def on_canvas_drag(self, event):
        if self.selected_element:
            x = self.canvas.canvasx(event.x) / self.canvas_scale - self.drag_data["x"]
            y = self.canvas.canvasy(event.y) / self.canvas_scale - self.drag_data["y"]

            # Snap to grid (10px)
            x = round(x / 10) * 10
            y = round(y / 10) * 10

            self.selected_element.x = max(0, min(x, self.canvas_width - self.selected_element.width))
            self.selected_element.y = max(0, min(y, self.canvas_height - self.selected_element.height))

            self.refresh_canvas()
            self.show_properties()
            self.modified = True
            self.update_title()

    def on_canvas_release(self, event):
        pass

    def on_list_select(self, event):
        selection = self.elem_listbox.curselection()
        if selection:
            self.selected_element = self.elements[selection[0]]
            self.refresh_canvas()
            self.show_properties()

    def show_properties(self):
        if self.selected_element:
            elem = self.selected_element

            # Basic properties
            self.name_entry.delete(0, tk.END)
            self.name_entry.insert(0, elem.name)
            self.text_entry.delete(0, tk.END)
            self.text_entry.insert(0, elem.text)
            self.x_entry.delete(0, tk.END)
            self.x_entry.insert(0, str(int(elem.x)))
            self.y_entry.delete(0, tk.END)
            self.y_entry.insert(0, str(int(elem.y)))
            self.width_entry.delete(0, tk.END)
            self.width_entry.insert(0, str(int(elem.width)))
            self.height_entry.delete(0, tk.END)
            self.height_entry.insert(0, str(int(elem.height)))
            self.filepath_entry.delete(0, tk.END)
            self.filepath_entry.insert(0, elem.filepath)
            self.btnid_entry.delete(0, tk.END)
            self.btnid_entry.insert(0, str(elem.button_id))

            # Scale
            self.scale_x_entry.delete(0, tk.END)
            self.scale_x_entry.insert(0, str(elem.scale[0]))
            self.scale_y_entry.delete(0, tk.END)
            self.scale_y_entry.insert(0, str(elem.scale[1]))

            # Color preview
            r, g, b = elem.color[0], elem.color[1], elem.color[2]
            color_hex = f"#{r:02x}{g:02x}{b:02x}"
            self.color_preview.configure(bg=color_hex)

            # Centered
            self.centered_var.set(elem.centered)

            # Navigation
            self.nav_up_combo.set(elem.nav_up)
            self.nav_down_combo.set(elem.nav_down)
            self.nav_left_combo.set(elem.nav_left)
            self.nav_right_combo.set(elem.nav_right)

            # Animation lists
            self.refresh_animation_lists()

    def refresh_animation_lists(self):
        """Refresh the animation keyframe lists"""
        self.enter_anim_list.delete(0, tk.END)
        self.exit_anim_list.delete(0, tk.END)

        if self.selected_element:
            for i, kf in enumerate(self.selected_element.enter_keyframes):
                pos_str = f"({kf.target_x}, {kf.target_y})" if kf.use_position else ""
                alpha_str = f"a={kf.target_alpha}" if kf.use_alpha else ""
                self.enter_anim_list.insert(tk.END, f"{i+1}: {kf.duration}ms {pos_str} {alpha_str}")

            for i, kf in enumerate(self.selected_element.exit_keyframes):
                pos_str = f"({kf.target_x}, {kf.target_y})" if kf.use_position else ""
                alpha_str = f"a={kf.target_alpha}" if kf.use_alpha else ""
                self.exit_anim_list.insert(tk.END, f"{i+1}: {kf.duration}ms {pos_str} {alpha_str}")

    def browse_filepath(self):
        """Browse for an image file"""
        initial_dir = self.game_root if self.game_root else os.getcwd()
        filepath = filedialog.askopenfilename(
            title="Select Image",
            initialdir=initial_dir,
            filetypes=[("PNG files", "*.png"), ("All files", "*.*")]
        )
        if filepath:
            # Make path relative to game root if possible
            if self.game_root and filepath.startswith(self.game_root):
                filepath = os.path.relpath(filepath, self.game_root)
            self.filepath_entry.delete(0, tk.END)
            self.filepath_entry.insert(0, filepath)

    def pick_color(self):
        """Open color picker dialog"""
        if self.selected_element:
            r, g, b = self.selected_element.color[0], self.selected_element.color[1], self.selected_element.color[2]
            initial_color = f"#{r:02x}{g:02x}{b:02x}"
            color = colorchooser.askcolor(color=initial_color, title="Choose Color")
            if color[0]:
                r, g, b = [int(c) for c in color[0]]
                self.selected_element.color = [r, g, b, self.selected_element.color[3]]
                self.color_preview.configure(bg=color[1])
                self.modified = True
                self.update_title()

    def apply_properties(self):
        if not self.selected_element:
            return

        elem = self.selected_element
        new_name = self.name_entry.get().strip()

        # Check for duplicate names
        if new_name != elem.name and any(e.name == new_name for e in self.elements):
            messagebox.showwarning("Warning", f"Element '{new_name}' already exists")
            return

        elem.name = new_name
        elem.text = self.text_entry.get()
        elem.x = int(self.x_entry.get() or 0)
        elem.y = int(self.y_entry.get() or 0)
        elem.width = int(self.width_entry.get() or 100)
        elem.height = int(self.height_entry.get() or 30)
        elem.filepath = self.filepath_entry.get()
        elem.button_id = int(self.btnid_entry.get() or 0)
        elem.scale = [float(self.scale_x_entry.get() or 1.0), float(self.scale_y_entry.get() or 1.0)]
        elem.centered = self.centered_var.get()

        # Clear image cache for this element if filepath changed
        self.image_cache = {k: v for k, v in self.image_cache.items() if elem.filepath not in k}

        self.refresh_canvas()
        self.refresh_list()
        self.modified = True
        self.update_title()

    def apply_navigation(self):
        """Apply navigation settings to selected element"""
        if not self.selected_element:
            return

        elem = self.selected_element
        elem.nav_up = self.nav_up_combo.get()
        elem.nav_down = self.nav_down_combo.get()
        elem.nav_left = self.nav_left_combo.get()
        elem.nav_right = self.nav_right_combo.get()

        self.modified = True
        self.update_title()

    def add_keyframe(self, anim_type):
        """Add a keyframe to the selected element's animation"""
        if not self.selected_element:
            messagebox.showwarning("Warning", "Select an element first")
            return

        dialog = KeyframeDialog(self.root, "Add Keyframe")
        if dialog.result:
            kf = AnimationKeyframe(dialog.result["duration"])
            kf.target_x = dialog.result["target_x"]
            kf.target_y = dialog.result["target_y"]
            kf.target_alpha = dialog.result["target_alpha"]
            kf.use_position = dialog.result["use_position"]
            kf.use_alpha = dialog.result["use_alpha"]

            if anim_type == "enter":
                self.selected_element.enter_keyframes.append(kf)
            else:
                self.selected_element.exit_keyframes.append(kf)

            self.refresh_animation_lists()
            self.modified = True
            self.update_title()

    def edit_keyframe(self, anim_type):
        """Edit the selected keyframe"""
        if not self.selected_element:
            return

        listbox = self.enter_anim_list if anim_type == "enter" else self.exit_anim_list
        keyframes = self.selected_element.enter_keyframes if anim_type == "enter" else self.selected_element.exit_keyframes

        selection = listbox.curselection()
        if not selection:
            messagebox.showwarning("Warning", "Select a keyframe to edit")
            return

        idx = selection[0]
        kf = keyframes[idx]

        dialog = KeyframeDialog(self.root, "Edit Keyframe", kf)
        if dialog.result:
            kf.duration = dialog.result["duration"]
            kf.target_x = dialog.result["target_x"]
            kf.target_y = dialog.result["target_y"]
            kf.target_alpha = dialog.result["target_alpha"]
            kf.use_position = dialog.result["use_position"]
            kf.use_alpha = dialog.result["use_alpha"]

            self.refresh_animation_lists()
            self.modified = True
            self.update_title()

    def delete_keyframe(self, anim_type):
        """Delete the selected keyframe"""
        if not self.selected_element:
            return

        listbox = self.enter_anim_list if anim_type == "enter" else self.exit_anim_list
        keyframes = self.selected_element.enter_keyframes if anim_type == "enter" else self.selected_element.exit_keyframes

        selection = listbox.curselection()
        if not selection:
            return

        idx = selection[0]
        del keyframes[idx]

        self.refresh_animation_lists()
        self.modified = True
        self.update_title()

    def delete_selected(self):
        if self.selected_element:
            if messagebox.askyesno("Confirm", f"Delete '{self.selected_element.name}'?"):
                self.elements.remove(self.selected_element)
                self.selected_element = None
                self.refresh_canvas()
                self.refresh_list()
                self.modified = True
                self.update_title()

    def auto_assign_navigation(self):
        """Automatically assign up/down navigation based on Y position"""
        buttons = sorted([e for e in self.elements if e.elem_type == "button"], key=lambda e: e.y)
        for i, btn in enumerate(buttons):
            btn.nav_up = buttons[i - 1].name if i > 0 else buttons[-1].name
            btn.nav_down = buttons[i + 1].name if i < len(buttons) - 1 else buttons[0].name
        self.modified = True
        self.update_title()
        messagebox.showinfo("Navigation", f"Assigned navigation for {len(buttons)} buttons")

    def edit_menu_settings(self):
        name = simpledialog.askstring("Menu Settings", "Menu name:", initialvalue=self.menu_name)
        if name:
            self.menu_name = name
            self.modified = True
            self.update_title()

    def edit_canvas_size(self):
        dialog = tk.Toplevel(self.root)
        dialog.title("Canvas Size")
        dialog.geometry("250x150")
        dialog.transient(self.root)
        dialog.grab_set()

        ttk.Label(dialog, text="Width:").grid(row=0, column=0, padx=5, pady=5)
        width_entry = ttk.Entry(dialog, width=10)
        width_entry.insert(0, str(self.canvas_width))
        width_entry.grid(row=0, column=1, padx=5, pady=5)

        ttk.Label(dialog, text="Height:").grid(row=1, column=0, padx=5, pady=5)
        height_entry = ttk.Entry(dialog, width=10)
        height_entry.insert(0, str(self.canvas_height))
        height_entry.grid(row=1, column=1, padx=5, pady=5)

        # Preset buttons
        preset_frame = ttk.Frame(dialog)
        preset_frame.grid(row=2, column=0, columnspan=2, pady=5)

        def set_preset(w, h):
            width_entry.delete(0, tk.END)
            width_entry.insert(0, str(w))
            height_entry.delete(0, tk.END)
            height_entry.insert(0, str(h))

        ttk.Button(preset_frame, text="720p", command=lambda: set_preset(1280, 720)).pack(side=tk.LEFT, padx=2)
        ttk.Button(preset_frame, text="1080p", command=lambda: set_preset(1920, 1080)).pack(side=tk.LEFT, padx=2)
        ttk.Button(preset_frame, text="2x", command=lambda: set_preset(2560, 1440)).pack(side=tk.LEFT, padx=2)

        def apply():
            self.canvas_width = int(width_entry.get())
            self.canvas_height = int(height_entry.get())
            self.set_zoom(self.canvas_scale)
            dialog.destroy()

        ttk.Button(dialog, text="Apply", command=apply).grid(row=3, column=0, columnspan=2, pady=10)

    def new_menu(self):
        if self.modified and not self.confirm_discard():
            return
        self.current_file = None
        self.menu_name = "MyMenu"
        self.elements = []
        self.selected_element = None
        self.refresh_canvas()
        self.refresh_list()
        self.modified = False
        self.update_title()

    def open_file(self):
        if self.modified and not self.confirm_discard():
            return
        filepath = filedialog.askopenfilename(
            title="Open Menu Layout",
            filetypes=[("Menu files", "*.menu"), ("JSON files", "*.json"), ("All files", "*.*")]
        )
        if filepath:
            self.load_file(filepath)

    def load_file(self, filepath):
        try:
            with open(filepath, 'r', encoding='utf-8') as f:
                data = json.load(f)
            self.menu_name = data.get("name", "MyMenu")
            self.canvas_width = data.get("width", 1280)
            self.canvas_height = data.get("height", 720)
            self.elements = [MenuElement.from_dict(e) for e in data.get("elements", [])]

            # Load templates and slots
            self.templates = [MenuTemplate.from_dict(t) for t in data.get("templates", [])]
            self.slots = [MenuSlot.from_dict(s) for s in data.get("slots", [])]

            # Load menu properties
            self.can_escape_from = data.get("canEscapeFrom", True)
            self.is_dynamic = data.get("isDynamic", False)
            self.use_mouse = data.get("useMouse", False)
            self.remember_last_button = data.get("rememberLastButton", False)

            self.set_zoom(self.canvas_scale)
            self.current_file = filepath
            self.selected_element = None
            self.selected_template = None
            self.selected_slot = None
            self.refresh_canvas()
            self.refresh_list()
            self.refresh_templates_list()
            self.refresh_slots_list()
            self.modified = False
            self.update_title()
            self.status_var.set(f"Loaded: {filepath}")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to load file:\n{e}")

    def save_file(self):
        if self.current_file:
            self.write_file(self.current_file)
        else:
            self.save_file_as()

    def save_file_as(self):
        initial_dir = os.path.join(self.game_root, "data", "menus") if self.game_root else None
        filepath = filedialog.asksaveasfilename(
            title="Save Menu Layout",
            initialdir=initial_dir,
            filetypes=[("Menu files", "*.menu"), ("JSON files", "*.json"), ("All files", "*.*")],
            defaultextension=".menu"
        )
        if filepath:
            self.write_file(filepath)
            self.current_file = filepath
            self.update_title()

    def write_file(self, filepath):
        try:
            data = {
                "name": self.menu_name,
                "width": self.canvas_width,
                "height": self.canvas_height,
                "canEscapeFrom": self.can_escape_from,
                "isDynamic": self.is_dynamic,
                "useMouse": self.use_mouse,
                "rememberLastButton": self.remember_last_button,
                "elements": [e.to_dict() for e in self.elements]
            }

            # Add templates if any
            if self.templates:
                data["templates"] = [t.to_dict() for t in self.templates]

            # Add slots if any
            if self.slots:
                data["slots"] = [s.to_dict() for s in self.slots]

            with open(filepath, 'w', encoding='utf-8') as f:
                json.dump(data, f, indent=2)
            self.modified = False
            self.update_title()
            self.status_var.set(f"Saved: {filepath}")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to save file:\n{e}")

    def export_cpp(self):
        """Export C++ code for CreateMenu function"""
        filepath = filedialog.asksaveasfilename(
            title="Export C++ Code",
            filetypes=[("C++ files", "*.cpp"), ("Text files", "*.txt")],
            defaultextension=".cpp"
        )
        if not filepath:
            return

        lines = []
        lines.append(f"// Generated menu code for: {self.menu_name}")
        lines.append(f"// Canvas size: {self.canvas_width}x{self.canvas_height}")
        lines.append("")
        lines.append(f"// Add to your CreateMenu function when name == \"{self.menu_name}\"")
        lines.append("")

        # Generate image code first (drawn behind buttons)
        images = [e for e in self.elements if e.elem_type == "image"]
        if images:
            lines.append("// Images")
            for img in images:
                var_name = img.name.replace(" ", "_").replace("-", "_")
                lines.append(f'Entity* {var_name} = AddImage("{img.filepath}",')
                lines.append(f'    glm::vec3({int(img.x)}, {int(img.y)}, 0),')
                lines.append(f'    glm::vec2({img.scale[0]}f, {img.scale[1]}f), game, 2);')

                # Generate enter/exit animation code
                if img.enter_keyframes or img.exit_keyframes:
                    lines.append("")
                    self._generate_animation_code(lines, var_name, img)
            lines.append("")

        # Generate text code
        texts = [e for e in self.elements if e.elem_type == "text"]
        if texts:
            lines.append("// Texts")
            for txt in texts:
                centered = "true" if txt.centered else "false"
                lines.append(f'AddText(game.theFont, "{txt.text}",')
                lines.append(f'    {int(txt.x)}, {int(txt.y)}, {txt.scale[0]}f, {txt.scale[1]}f, {centered});')
            lines.append("")

        # Generate button code
        buttons = [e for e in self.elements if e.elem_type == "button"]
        if buttons:
            lines.append("// Buttons")
            for btn in buttons:
                var_name = btn.name.replace(" ", "_").replace("-", "_")
                filepath_str = f'"{btn.filepath}"' if btn.filepath else '"assets/gui/menu.png"'
                r, g, b, a = btn.color
                lines.append(f'MenuButton* {var_name} = AddButton("{btn.text}", {filepath_str},')
                lines.append(f'    {btn.button_id}, glm::vec3({int(btn.x)}, {int(btn.y)}, 0), game, {{{r}, {g}, {b}, {a}}});')

                if btn.scale != [1.0, 1.0]:
                    lines.append(f'{var_name}->scale = glm::vec2({btn.scale[0]}f, {btn.scale[1]}f);')

                # Generate enter/exit animation code
                if btn.enter_keyframes or btn.exit_keyframes:
                    self._generate_animation_code(lines, var_name, btn)
            lines.append("")

        # Generate navigation code
        if buttons:
            lines.append("// Button navigation")
            for btn in buttons:
                var_name = btn.name.replace(" ", "_").replace("-", "_")
                if btn.nav_up:
                    up_var = btn.nav_up.replace(" ", "_").replace("-", "_")
                    lines.append(f'{var_name}->up = {up_var};')
                if btn.nav_down:
                    down_var = btn.nav_down.replace(" ", "_").replace("-", "_")
                    lines.append(f'{var_name}->down = {down_var};')
                if btn.nav_left:
                    left_var = btn.nav_left.replace(" ", "_").replace("-", "_")
                    lines.append(f'{var_name}->left = {left_var};')
                if btn.nav_right:
                    right_var = btn.nav_right.replace(" ", "_").replace("-", "_")
                    lines.append(f'{var_name}->right = {right_var};')
            lines.append("")
            lines.append("// Or use auto-assignment:")
            lines.append("// AssignButtons(true);")

        try:
            with open(filepath, 'w', encoding='utf-8') as f:
                f.write('\n'.join(lines))
            self.status_var.set(f"Exported: {filepath}")
            messagebox.showinfo("Export Complete", f"C++ code exported to:\n{filepath}")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to export:\n{e}")

    def _generate_animation_code(self, lines, var_name, elem):
        """Generate C++ code for element animations"""
        if elem.enter_keyframes:
            lines.append(f'MenuAnimation* {var_name}AnimEnter = CreateEnterAnimation({var_name});')
            for kf in elem.enter_keyframes:
                lines.append(f'{{')
                lines.append(f'    MenuAnimKeyframe* kf = {var_name}AnimEnter->CreateKeyframe({kf.duration});')
                if kf.use_position:
                    lines.append(f'    kf->targetPosition = glm::vec3({kf.target_x}, {kf.target_y}, 0);')
                if kf.use_alpha:
                    lines.append(f'    kf->targetColor = {{255, 255, 255, {kf.target_alpha}}};')
                lines.append(f'}}')

        if elem.exit_keyframes:
            lines.append(f'MenuAnimation* {var_name}AnimExit = CreateExitAnimation({var_name});')
            for kf in elem.exit_keyframes:
                lines.append(f'{{')
                lines.append(f'    MenuAnimKeyframe* kf = {var_name}AnimExit->CreateKeyframe({kf.duration});')
                if kf.use_position:
                    lines.append(f'    kf->targetPosition = glm::vec3({kf.target_x}, {kf.target_y}, 0);')
                if kf.use_alpha:
                    lines.append(f'    kf->targetColor = {{255, 255, 255, {kf.target_alpha}}};')
                lines.append(f'}}')

    def export_menu_data(self):
        """Export to .menu data file format that can be loaded at runtime"""
        initial_dir = os.path.join(self.game_root, "data", "menus") if self.game_root else None

        # Create menus directory if it doesn't exist
        if initial_dir and not os.path.exists(initial_dir):
            os.makedirs(initial_dir)

        filepath = filedialog.asksaveasfilename(
            title="Export Menu Data",
            initialdir=initial_dir,
            filetypes=[("Menu files", "*.menu")],
            defaultextension=".menu",
            initialfile=f"{self.menu_name}.menu"
        )
        if filepath:
            self.write_file(filepath)
            messagebox.showinfo("Export Complete", f"Menu data exported to:\n{filepath}")

    def confirm_discard(self):
        return messagebox.askyesno("Unsaved Changes", "You have unsaved changes. Discard them?")

    def update_title(self):
        title = f"Menu Editor - {self.menu_name}"
        if self.current_file:
            title += f" ({os.path.basename(self.current_file)})"
        if self.modified:
            title += " *"
        self.root.title(title)

    def on_close(self):
        if self.modified and not self.confirm_discard():
            return
        self.root.destroy()

    # Template methods
    def refresh_templates_list(self):
        """Refresh the templates listbox"""
        self.templates_list.delete(0, tk.END)
        for t in self.templates:
            self.templates_list.insert(tk.END, f"{t.name} ({t.data_source})")

    def on_template_select(self, event):
        """Handle template selection"""
        selection = self.templates_list.curselection()
        if selection:
            self.selected_template = self.templates[selection[0]]

    def add_template(self):
        """Add a new template"""
        dialog = TemplateDialog(self.root, "Add Template")
        if dialog.result:
            t = MenuTemplate(dialog.result["name"])
            t.element_type = dialog.result["element_type"]
            t.data_source = dialog.result["data_source"]
            t.filter = dialog.result["filter"]
            t.layout = dialog.result["layout"]
            t.columns = dialog.result["columns"]
            t.start_x = dialog.result["start_x"]
            t.start_y = dialog.result["start_y"]
            t.spacing_x = dialog.result["spacing_x"]
            t.spacing_y = dialog.result["spacing_y"]
            t.text = dialog.result["text"]
            t.filepath_pattern = dialog.result["filepath_pattern"]
            self.templates.append(t)
            self.refresh_templates_list()
            self.modified = True
            self.update_title()

    def edit_template(self):
        """Edit the selected template"""
        if not self.selected_template:
            messagebox.showwarning("Warning", "Select a template to edit")
            return

        dialog = TemplateDialog(self.root, "Edit Template", self.selected_template)
        if dialog.result:
            t = self.selected_template
            t.name = dialog.result["name"]
            t.element_type = dialog.result["element_type"]
            t.data_source = dialog.result["data_source"]
            t.filter = dialog.result["filter"]
            t.layout = dialog.result["layout"]
            t.columns = dialog.result["columns"]
            t.start_x = dialog.result["start_x"]
            t.start_y = dialog.result["start_y"]
            t.spacing_x = dialog.result["spacing_x"]
            t.spacing_y = dialog.result["spacing_y"]
            t.text = dialog.result["text"]
            t.filepath_pattern = dialog.result["filepath_pattern"]
            self.refresh_templates_list()
            self.modified = True
            self.update_title()

    def delete_template(self):
        """Delete the selected template"""
        if self.selected_template:
            if messagebox.askyesno("Confirm", f"Delete template '{self.selected_template.name}'?"):
                self.templates.remove(self.selected_template)
                self.selected_template = None
                self.refresh_templates_list()
                self.modified = True
                self.update_title()

    # Slot methods
    def refresh_slots_list(self):
        """Refresh the slots listbox"""
        self.slots_list.delete(0, tk.END)
        for s in self.slots:
            self.slots_list.insert(tk.END, f"{s.name} ({s.layout})")

    def on_slot_select(self, event):
        """Handle slot selection"""
        selection = self.slots_list.curselection()
        if selection:
            self.selected_slot = self.slots[selection[0]]

    def add_slot(self):
        """Add a new slot"""
        dialog = SlotDialog(self.root, "Add Slot")
        if dialog.result:
            s = MenuSlot(dialog.result["name"])
            s.x = dialog.result["x"]
            s.y = dialog.result["y"]
            s.layout = dialog.result["layout"]
            s.columns = dialog.result["columns"]
            s.spacing_x = dialog.result["spacing_x"]
            s.spacing_y = dialog.result["spacing_y"]
            self.slots.append(s)
            self.refresh_slots_list()
            self.modified = True
            self.update_title()

    def edit_slot(self):
        """Edit the selected slot"""
        if not self.selected_slot:
            messagebox.showwarning("Warning", "Select a slot to edit")
            return

        dialog = SlotDialog(self.root, "Edit Slot", self.selected_slot)
        if dialog.result:
            s = self.selected_slot
            s.name = dialog.result["name"]
            s.x = dialog.result["x"]
            s.y = dialog.result["y"]
            s.layout = dialog.result["layout"]
            s.columns = dialog.result["columns"]
            s.spacing_x = dialog.result["spacing_x"]
            s.spacing_y = dialog.result["spacing_y"]
            self.refresh_slots_list()
            self.modified = True
            self.update_title()

    def delete_slot(self):
        """Delete the selected slot"""
        if self.selected_slot:
            if messagebox.askyesno("Confirm", f"Delete slot '{self.selected_slot.name}'?"):
                self.slots.remove(self.selected_slot)
                self.selected_slot = None
                self.refresh_slots_list()
                self.modified = True
                self.update_title()


class TemplateDialog:
    """Dialog for editing menu templates"""
    def __init__(self, parent, title, template=None):
        self.result = None

        dialog = tk.Toplevel(parent)
        dialog.title(title)
        dialog.geometry("400x450")
        dialog.transient(parent)
        dialog.grab_set()

        row = 0

        ttk.Label(dialog, text="Name:").grid(row=row, column=0, padx=5, pady=2, sticky=tk.W)
        self.name_entry = ttk.Entry(dialog, width=25)
        self.name_entry.grid(row=row, column=1, padx=5, pady=2)
        self.name_entry.insert(0, template.name if template else "")

        row += 1
        ttk.Label(dialog, text="Element Type:").grid(row=row, column=0, padx=5, pady=2, sticky=tk.W)
        self.type_combo = ttk.Combobox(dialog, values=["button", "text", "image"], width=22, state="readonly")
        self.type_combo.grid(row=row, column=1, padx=5, pady=2)
        self.type_combo.set(template.element_type if template else "button")

        row += 1
        ttk.Label(dialog, text="Data Source:").grid(row=row, column=0, padx=5, pady=2, sticky=tk.W)
        self.datasource_entry = ttk.Entry(dialog, width=25)
        self.datasource_entry.grid(row=row, column=1, padx=5, pady=2)
        self.datasource_entry.insert(0, template.data_source if template else "")

        row += 1
        ttk.Label(dialog, text="Filter:").grid(row=row, column=0, padx=5, pady=2, sticky=tk.W)
        self.filter_entry = ttk.Entry(dialog, width=25)
        self.filter_entry.grid(row=row, column=1, padx=5, pady=2)
        self.filter_entry.insert(0, template.filter if template else "")

        row += 1
        ttk.Label(dialog, text="Layout:").grid(row=row, column=0, padx=5, pady=2, sticky=tk.W)
        self.layout_combo = ttk.Combobox(dialog, values=["grid", "horizontal", "vertical"], width=22, state="readonly")
        self.layout_combo.grid(row=row, column=1, padx=5, pady=2)
        self.layout_combo.set(template.layout if template else "grid")

        row += 1
        ttk.Label(dialog, text="Columns:").grid(row=row, column=0, padx=5, pady=2, sticky=tk.W)
        self.columns_entry = ttk.Entry(dialog, width=10)
        self.columns_entry.grid(row=row, column=1, padx=5, pady=2, sticky=tk.W)
        self.columns_entry.insert(0, str(template.columns if template else 3))

        row += 1
        ttk.Label(dialog, text="Start X:").grid(row=row, column=0, padx=5, pady=2, sticky=tk.W)
        self.startx_entry = ttk.Entry(dialog, width=10)
        self.startx_entry.grid(row=row, column=1, padx=5, pady=2, sticky=tk.W)
        self.startx_entry.insert(0, str(template.start_x if template else 0))

        row += 1
        ttk.Label(dialog, text="Start Y:").grid(row=row, column=0, padx=5, pady=2, sticky=tk.W)
        self.starty_entry = ttk.Entry(dialog, width=10)
        self.starty_entry.grid(row=row, column=1, padx=5, pady=2, sticky=tk.W)
        self.starty_entry.insert(0, str(template.start_y if template else 0))

        row += 1
        ttk.Label(dialog, text="Spacing X:").grid(row=row, column=0, padx=5, pady=2, sticky=tk.W)
        self.spacingx_entry = ttk.Entry(dialog, width=10)
        self.spacingx_entry.grid(row=row, column=1, padx=5, pady=2, sticky=tk.W)
        self.spacingx_entry.insert(0, str(template.spacing_x if template else 240))

        row += 1
        ttk.Label(dialog, text="Spacing Y:").grid(row=row, column=0, padx=5, pady=2, sticky=tk.W)
        self.spacingy_entry = ttk.Entry(dialog, width=10)
        self.spacingy_entry.grid(row=row, column=1, padx=5, pady=2, sticky=tk.W)
        self.spacingy_entry.insert(0, str(template.spacing_y if template else 320))

        row += 1
        ttk.Label(dialog, text="Text Pattern:").grid(row=row, column=0, padx=5, pady=2, sticky=tk.W)
        self.text_entry = ttk.Entry(dialog, width=25)
        self.text_entry.grid(row=row, column=1, padx=5, pady=2)
        self.text_entry.insert(0, template.text if template else "{name}")

        row += 1
        ttk.Label(dialog, text="Filepath Pattern:").grid(row=row, column=0, padx=5, pady=2, sticky=tk.W)
        self.filepath_entry = ttk.Entry(dialog, width=25)
        self.filepath_entry.grid(row=row, column=1, padx=5, pady=2)
        self.filepath_entry.insert(0, template.filepath_pattern if template else "")

        row += 1
        btn_frame = ttk.Frame(dialog)
        btn_frame.grid(row=row, column=0, columnspan=2, pady=15)
        ttk.Button(btn_frame, text="OK", command=lambda: self.on_ok(dialog)).pack(side=tk.LEFT, padx=5)
        ttk.Button(btn_frame, text="Cancel", command=dialog.destroy).pack(side=tk.LEFT, padx=5)

        dialog.wait_window()

    def on_ok(self, dialog):
        self.result = {
            "name": self.name_entry.get(),
            "element_type": self.type_combo.get(),
            "data_source": self.datasource_entry.get(),
            "filter": self.filter_entry.get(),
            "layout": self.layout_combo.get(),
            "columns": int(self.columns_entry.get() or 3),
            "start_x": float(self.startx_entry.get() or 0),
            "start_y": float(self.starty_entry.get() or 0),
            "spacing_x": float(self.spacingx_entry.get() or 240),
            "spacing_y": float(self.spacingy_entry.get() or 320),
            "text": self.text_entry.get(),
            "filepath_pattern": self.filepath_entry.get()
        }
        dialog.destroy()


class SlotDialog:
    """Dialog for editing menu slots"""
    def __init__(self, parent, title, slot=None):
        self.result = None

        dialog = tk.Toplevel(parent)
        dialog.title(title)
        dialog.geometry("300x280")
        dialog.transient(parent)
        dialog.grab_set()

        row = 0

        ttk.Label(dialog, text="Name:").grid(row=row, column=0, padx=5, pady=5, sticky=tk.W)
        self.name_entry = ttk.Entry(dialog, width=20)
        self.name_entry.grid(row=row, column=1, padx=5, pady=5)
        self.name_entry.insert(0, slot.name if slot else "")

        row += 1
        ttk.Label(dialog, text="X:").grid(row=row, column=0, padx=5, pady=5, sticky=tk.W)
        self.x_entry = ttk.Entry(dialog, width=10)
        self.x_entry.grid(row=row, column=1, padx=5, pady=5, sticky=tk.W)
        self.x_entry.insert(0, str(slot.x if slot else 0))

        row += 1
        ttk.Label(dialog, text="Y:").grid(row=row, column=0, padx=5, pady=5, sticky=tk.W)
        self.y_entry = ttk.Entry(dialog, width=10)
        self.y_entry.grid(row=row, column=1, padx=5, pady=5, sticky=tk.W)
        self.y_entry.insert(0, str(slot.y if slot else 0))

        row += 1
        ttk.Label(dialog, text="Layout:").grid(row=row, column=0, padx=5, pady=5, sticky=tk.W)
        self.layout_combo = ttk.Combobox(dialog, values=["vertical", "horizontal", "grid"], width=17, state="readonly")
        self.layout_combo.grid(row=row, column=1, padx=5, pady=5)
        self.layout_combo.set(slot.layout if slot else "vertical")

        row += 1
        ttk.Label(dialog, text="Columns:").grid(row=row, column=0, padx=5, pady=5, sticky=tk.W)
        self.columns_entry = ttk.Entry(dialog, width=10)
        self.columns_entry.grid(row=row, column=1, padx=5, pady=5, sticky=tk.W)
        self.columns_entry.insert(0, str(slot.columns if slot else 1))

        row += 1
        ttk.Label(dialog, text="Spacing X:").grid(row=row, column=0, padx=5, pady=5, sticky=tk.W)
        self.spacingx_entry = ttk.Entry(dialog, width=10)
        self.spacingx_entry.grid(row=row, column=1, padx=5, pady=5, sticky=tk.W)
        self.spacingx_entry.insert(0, str(slot.spacing_x if slot else 0))

        row += 1
        ttk.Label(dialog, text="Spacing Y:").grid(row=row, column=0, padx=5, pady=5, sticky=tk.W)
        self.spacingy_entry = ttk.Entry(dialog, width=10)
        self.spacingy_entry.grid(row=row, column=1, padx=5, pady=5, sticky=tk.W)
        self.spacingy_entry.insert(0, str(slot.spacing_y if slot else 120))

        row += 1
        btn_frame = ttk.Frame(dialog)
        btn_frame.grid(row=row, column=0, columnspan=2, pady=15)
        ttk.Button(btn_frame, text="OK", command=lambda: self.on_ok(dialog)).pack(side=tk.LEFT, padx=5)
        ttk.Button(btn_frame, text="Cancel", command=dialog.destroy).pack(side=tk.LEFT, padx=5)

        dialog.wait_window()

    def on_ok(self, dialog):
        self.result = {
            "name": self.name_entry.get(),
            "x": float(self.x_entry.get() or 0),
            "y": float(self.y_entry.get() or 0),
            "layout": self.layout_combo.get(),
            "columns": int(self.columns_entry.get() or 1),
            "spacing_x": float(self.spacingx_entry.get() or 0),
            "spacing_y": float(self.spacingy_entry.get() or 120)
        }
        dialog.destroy()


class KeyframeDialog:
    """Dialog for editing animation keyframes"""
    def __init__(self, parent, title, keyframe=None):
        self.result = None

        dialog = tk.Toplevel(parent)
        dialog.title(title)
        dialog.geometry("300x250")
        dialog.transient(parent)
        dialog.grab_set()

        # Duration
        ttk.Label(dialog, text="Duration (ms):").grid(row=0, column=0, padx=5, pady=5, sticky=tk.W)
        self.duration_entry = ttk.Entry(dialog, width=10)
        self.duration_entry.grid(row=0, column=1, padx=5, pady=5)
        self.duration_entry.insert(0, str(keyframe.duration if keyframe else 500))

        # Position
        self.use_pos_var = tk.BooleanVar(value=keyframe.use_position if keyframe else True)
        ttk.Checkbutton(dialog, text="Change Position", variable=self.use_pos_var).grid(
            row=1, column=0, columnspan=2, padx=5, pady=5, sticky=tk.W)

        ttk.Label(dialog, text="Target X:").grid(row=2, column=0, padx=5, pady=2, sticky=tk.W)
        self.target_x_entry = ttk.Entry(dialog, width=10)
        self.target_x_entry.grid(row=2, column=1, padx=5, pady=2)
        self.target_x_entry.insert(0, str(keyframe.target_x if keyframe else 0))

        ttk.Label(dialog, text="Target Y:").grid(row=3, column=0, padx=5, pady=2, sticky=tk.W)
        self.target_y_entry = ttk.Entry(dialog, width=10)
        self.target_y_entry.grid(row=3, column=1, padx=5, pady=2)
        self.target_y_entry.insert(0, str(keyframe.target_y if keyframe else 0))

        # Alpha
        self.use_alpha_var = tk.BooleanVar(value=keyframe.use_alpha if keyframe else False)
        ttk.Checkbutton(dialog, text="Change Alpha", variable=self.use_alpha_var).grid(
            row=4, column=0, columnspan=2, padx=5, pady=5, sticky=tk.W)

        ttk.Label(dialog, text="Target Alpha:").grid(row=5, column=0, padx=5, pady=2, sticky=tk.W)
        self.target_alpha_entry = ttk.Entry(dialog, width=10)
        self.target_alpha_entry.grid(row=5, column=1, padx=5, pady=2)
        self.target_alpha_entry.insert(0, str(keyframe.target_alpha if keyframe else 255))

        # Buttons
        btn_frame = ttk.Frame(dialog)
        btn_frame.grid(row=6, column=0, columnspan=2, pady=15)

        ttk.Button(btn_frame, text="OK", command=lambda: self.on_ok(dialog)).pack(side=tk.LEFT, padx=5)
        ttk.Button(btn_frame, text="Cancel", command=dialog.destroy).pack(side=tk.LEFT, padx=5)

        dialog.wait_window()

    def on_ok(self, dialog):
        self.result = {
            "duration": int(self.duration_entry.get() or 500),
            "target_x": int(self.target_x_entry.get() or 0),
            "target_y": int(self.target_y_entry.get() or 0),
            "target_alpha": int(self.target_alpha_entry.get() or 255),
            "use_position": self.use_pos_var.get(),
            "use_alpha": self.use_alpha_var.get()
        }
        dialog.destroy()


def main():
    root = tk.Tk()
    app = MenuEditor(root)
    root.protocol("WM_DELETE_WINDOW", app.on_close)
    root.mainloop()


if __name__ == "__main__":
    main()
