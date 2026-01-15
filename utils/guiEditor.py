"""
GUI Editor - Visual designer for in-game GUI/HUD layouts
Supports both standalone JSON layouts and engine gui.dat format.

Features:
1. Visual element positioning with drag & drop
2. Preview actual images from game assets
3. Import/export gui.dat format
4. Export C++ code snippets
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


class GUIElement:
    ANCHORS = ["top-left", "top-center", "top-right",
               "center-left", "center", "center-right",
               "bottom-left", "bottom-center", "bottom-right"]

    def __init__(self, elem_type, name, x, y, width=100, height=30):
        self.elem_type = elem_type  # "text", "image", "bar", "rect"
        self.name = name
        self.x = x
        self.y = y
        self.width = width
        self.height = height
        self.text = name
        self.filepath = ""
        self.font_size = 24
        self.color = (255, 255, 255, 255)
        self.bg_color = (0, 0, 0, 128)
        self.scale = (1.0, 1.0)
        self.anchor = "top-left"
        self.visible = True
        self.layer = 0  # Draw order
        # For engine gui.dat format
        self.z_order = 0

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
            "bg_color": self.bg_color,
            "scale": self.scale,
            "anchor": self.anchor,
            "visible": self.visible,
            "layer": self.layer,
            "z_order": self.z_order
        }

    @staticmethod
    def from_dict(data):
        elem = GUIElement(data["type"], data["name"], data["x"], data["y"],
                         data.get("width", 100), data.get("height", 30))
        elem.text = data.get("text", elem.name)
        elem.filepath = data.get("filepath", "")
        elem.font_size = data.get("font_size", 24)
        elem.color = tuple(data.get("color", (255, 255, 255, 255)))
        elem.bg_color = tuple(data.get("bg_color", (0, 0, 0, 128)))
        elem.scale = tuple(data.get("scale", (1.0, 1.0)))
        elem.anchor = data.get("anchor", "top-left")
        elem.visible = data.get("visible", True)
        elem.layer = data.get("layer", 0)
        elem.z_order = data.get("z_order", 0)
        return elem

    def get_anchor_offset(self, canvas_width, canvas_height):
        """Calculate position offset based on anchor"""
        ax, ay = 0, 0
        if "center" in self.anchor and "left" not in self.anchor and "right" not in self.anchor:
            ax = canvas_width // 2
        elif "right" in self.anchor:
            ax = canvas_width
        if "center" in self.anchor and "top" not in self.anchor and "bottom" not in self.anchor:
            ay = canvas_height // 2
        elif "bottom" in self.anchor:
            ay = canvas_height
        return ax, ay


class GUIEditor:
    def __init__(self, root):
        self.root = root
        self.root.title("GUI Editor")
        self.root.geometry("1300x850")

        # Project data
        self.game_root = None
        self.current_file = None
        self.layout_name = "GameGUI"
        self.elements = []
        self.selected_element = None
        self.drag_data = {"x": 0, "y": 0}
        self.modified = False

        # GUI screens from gui.dat
        self.gui_screens = {}  # screen_name -> list of elements
        self.current_screen = None

        # Image/text definitions from gui.dat
        self.image_defs = {}  # name -> filepath
        self.text_defs = {}   # name -> text content

        # Image cache for preview
        self.image_cache = {}  # filepath -> PhotoImage

        # Canvas dimensions (typical game resolution)
        self.canvas_width = 1280
        self.canvas_height = 720

        self.create_menu()
        self.create_main_area()
        self.create_status_bar()

    def create_menu(self):
        menubar = tk.Menu(self.root)
        self.root.config(menu=menubar)

        file_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="File", menu=file_menu)
        file_menu.add_command(label="Open Project...", command=self.open_project, accelerator="Ctrl+O")
        file_menu.add_separator()
        file_menu.add_command(label="New Layout", command=self.new_layout, accelerator="Ctrl+N")
        file_menu.add_command(label="Open Layout...", command=self.open_file)
        file_menu.add_command(label="Save Layout", command=self.save_file, accelerator="Ctrl+S")
        file_menu.add_command(label="Save Layout As...", command=self.save_file_as)
        file_menu.add_separator()
        file_menu.add_command(label="Import from gui.dat", command=self.import_gui_dat)
        file_menu.add_command(label="Export to gui.dat", command=self.export_gui_dat)
        file_menu.add_separator()
        file_menu.add_command(label="Export C++ Code...", command=self.export_cpp)
        file_menu.add_separator()
        file_menu.add_command(label="Exit", command=self.on_close)

        edit_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="Edit", menu=edit_menu)
        edit_menu.add_command(label="Layout Settings...", command=self.edit_layout_settings)
        edit_menu.add_command(label="Canvas Size...", command=self.edit_canvas_size)
        edit_menu.add_separator()
        edit_menu.add_command(label="Delete Selected", command=self.delete_selected, accelerator="Del")
        edit_menu.add_command(label="Duplicate", command=self.duplicate_selected, accelerator="Ctrl+D")

        view_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="View", menu=view_menu)
        self.show_guides = tk.BooleanVar(value=True)
        view_menu.add_checkbutton(label="Show Anchor Guides", variable=self.show_guides,
                                  command=self.refresh_canvas)
        self.show_images = tk.BooleanVar(value=True)
        view_menu.add_checkbutton(label="Show Image Previews", variable=self.show_images,
                                  command=self.refresh_canvas)

        self.root.bind("<Control-n>", lambda e: self.new_layout())
        self.root.bind("<Control-o>", lambda e: self.open_project())
        self.root.bind("<Control-s>", lambda e: self.save_file())
        self.root.bind("<Control-d>", lambda e: self.duplicate_selected())
        self.root.bind("<Delete>", lambda e: self.delete_selected())

    def create_main_area(self):
        main_paned = ttk.PanedWindow(self.root, orient=tk.HORIZONTAL)
        main_paned.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        # Left panel - Screens list (when project loaded)
        left_frame = ttk.Frame(main_paned)
        main_paned.add(left_frame, weight=1)

        # Project info
        project_frame = ttk.LabelFrame(left_frame, text="Project")
        project_frame.pack(fill=tk.X, padx=2, pady=2)

        self.project_path_var = tk.StringVar(value="No project open")
        ttk.Label(project_frame, textvariable=self.project_path_var,
                 wraplength=180, foreground="gray").pack(padx=5, pady=5)
        ttk.Button(project_frame, text="Open Project...", command=self.open_project).pack(padx=5, pady=2)

        # GUI Screens list
        screens_frame = ttk.LabelFrame(left_frame, text="GUI Screens")
        screens_frame.pack(fill=tk.BOTH, expand=True, padx=2, pady=2)

        screen_btn_frame = ttk.Frame(screens_frame)
        screen_btn_frame.pack(fill=tk.X, padx=2, pady=2)
        ttk.Button(screen_btn_frame, text="Add", command=self.add_screen).pack(side=tk.LEFT, padx=2)
        ttk.Button(screen_btn_frame, text="Delete", command=self.delete_screen).pack(side=tk.LEFT, padx=2)

        self.screen_listbox = tk.Listbox(screens_frame, exportselection=False)
        screen_scroll = ttk.Scrollbar(screens_frame, orient=tk.VERTICAL, command=self.screen_listbox.yview)
        self.screen_listbox.configure(yscrollcommand=screen_scroll.set)
        self.screen_listbox.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=2, pady=2)
        screen_scroll.pack(side=tk.RIGHT, fill=tk.Y, pady=2)
        self.screen_listbox.bind("<<ListboxSelect>>", self.on_screen_selected)

        # Middle panel - Canvas
        middle_frame = ttk.Frame(main_paned)
        main_paned.add(middle_frame, weight=4)

        # Toolbar
        toolbar = ttk.Frame(middle_frame)
        toolbar.pack(fill=tk.X, pady=2)

        ttk.Button(toolbar, text="+ Text", command=lambda: self.add_element("text")).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="+ Image", command=lambda: self.add_element("image")).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="+ Bar", command=lambda: self.add_element("bar")).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="+ Rect", command=lambda: self.add_element("rect")).pack(side=tk.LEFT, padx=2)

        ttk.Separator(toolbar, orient=tk.VERTICAL).pack(side=tk.LEFT, fill=tk.Y, padx=10)

        ttk.Label(toolbar, text="Screen:").pack(side=tk.LEFT, padx=2)
        self.screen_name_var = tk.StringVar(value="(none)")
        ttk.Label(toolbar, textvariable=self.screen_name_var, font=("Arial", 10, "bold")).pack(side=tk.LEFT, padx=5)

        # Canvas with scrollbars
        canvas_frame = ttk.Frame(middle_frame)
        canvas_frame.pack(fill=tk.BOTH, expand=True)

        self.canvas = tk.Canvas(canvas_frame, bg="#222222", width=self.canvas_width, height=self.canvas_height)
        h_scroll = ttk.Scrollbar(canvas_frame, orient=tk.HORIZONTAL, command=self.canvas.xview)
        v_scroll = ttk.Scrollbar(canvas_frame, orient=tk.VERTICAL, command=self.canvas.yview)
        self.canvas.configure(xscrollcommand=h_scroll.set, yscrollcommand=v_scroll.set)

        self.canvas.grid(row=0, column=0, sticky="nsew")
        v_scroll.grid(row=0, column=1, sticky="ns")
        h_scroll.grid(row=1, column=0, sticky="ew")
        canvas_frame.rowconfigure(0, weight=1)
        canvas_frame.columnconfigure(0, weight=1)

        self.canvas.configure(scrollregion=(0, 0, self.canvas_width, self.canvas_height))

        self.canvas.bind("<Button-1>", self.on_canvas_click)
        self.canvas.bind("<B1-Motion>", self.on_canvas_drag)
        self.canvas.bind("<ButtonRelease-1>", self.on_canvas_release)

        # Right panel - Properties
        right_frame = ttk.Frame(main_paned)
        main_paned.add(right_frame, weight=1)

        # Elements list
        list_frame = ttk.LabelFrame(right_frame, text="Elements")
        list_frame.pack(fill=tk.BOTH, expand=True)

        list_inner = ttk.Frame(list_frame)
        list_inner.pack(fill=tk.BOTH, expand=True)

        self.elem_listbox = tk.Listbox(list_inner, selectmode=tk.SINGLE)
        list_scroll = ttk.Scrollbar(list_inner, orient=tk.VERTICAL, command=self.elem_listbox.yview)
        self.elem_listbox.configure(yscrollcommand=list_scroll.set)
        self.elem_listbox.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=2, pady=2)
        list_scroll.pack(side=tk.RIGHT, fill=tk.Y)
        self.elem_listbox.bind("<<ListboxSelect>>", self.on_list_select)

        list_buttons = ttk.Frame(list_frame)
        list_buttons.pack(fill=tk.X)
        ttk.Button(list_buttons, text="Up", command=self.move_layer_up).pack(side=tk.LEFT, padx=2)
        ttk.Button(list_buttons, text="Down", command=self.move_layer_down).pack(side=tk.LEFT, padx=2)

        # Properties panel
        prop_frame = ttk.LabelFrame(right_frame, text="Properties")
        prop_frame.pack(fill=tk.X, pady=5)

        # Create scrollable properties
        prop_canvas = tk.Canvas(prop_frame, highlightthickness=0, height=350)
        prop_scroll = ttk.Scrollbar(prop_frame, orient=tk.VERTICAL, command=prop_canvas.yview)
        prop_inner = ttk.Frame(prop_canvas)

        prop_canvas.create_window((0, 0), window=prop_inner, anchor=tk.NW)
        prop_canvas.configure(yscrollcommand=prop_scroll.set)

        prop_canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        prop_scroll.pack(side=tk.RIGHT, fill=tk.Y)

        prop_inner.bind("<Configure>", lambda e: prop_canvas.configure(scrollregion=prop_canvas.bbox("all")))

        # Property entries
        row = 0

        ttk.Label(prop_inner, text="Name:").grid(row=row, column=0, sticky=tk.W, padx=2, pady=1)
        self.name_entry = ttk.Entry(prop_inner, width=18)
        self.name_entry.grid(row=row, column=1, sticky=tk.W+tk.E, padx=2, pady=1)
        row += 1

        ttk.Label(prop_inner, text="Type:").grid(row=row, column=0, sticky=tk.W, padx=2, pady=1)
        self.type_label = ttk.Label(prop_inner, text="-")
        self.type_label.grid(row=row, column=1, sticky=tk.W, padx=2, pady=1)
        row += 1

        ttk.Label(prop_inner, text="Text:").grid(row=row, column=0, sticky=tk.W, padx=2, pady=1)
        self.text_entry = ttk.Entry(prop_inner, width=18)
        self.text_entry.grid(row=row, column=1, sticky=tk.W+tk.E, padx=2, pady=1)
        row += 1

        ttk.Label(prop_inner, text="Position:").grid(row=row, column=0, sticky=tk.W, padx=2, pady=1)
        pos_frame = ttk.Frame(prop_inner)
        pos_frame.grid(row=row, column=1, sticky=tk.W, padx=2, pady=1)
        ttk.Label(pos_frame, text="X:").pack(side=tk.LEFT)
        self.x_entry = ttk.Entry(pos_frame, width=5)
        self.x_entry.pack(side=tk.LEFT, padx=1)
        ttk.Label(pos_frame, text="Y:").pack(side=tk.LEFT)
        self.y_entry = ttk.Entry(pos_frame, width=5)
        self.y_entry.pack(side=tk.LEFT, padx=1)
        row += 1

        ttk.Label(prop_inner, text="Size:").grid(row=row, column=0, sticky=tk.W, padx=2, pady=1)
        size_frame = ttk.Frame(prop_inner)
        size_frame.grid(row=row, column=1, sticky=tk.W, padx=2, pady=1)
        ttk.Label(size_frame, text="W:").pack(side=tk.LEFT)
        self.width_entry = ttk.Entry(size_frame, width=5)
        self.width_entry.pack(side=tk.LEFT, padx=1)
        ttk.Label(size_frame, text="H:").pack(side=tk.LEFT)
        self.height_entry = ttk.Entry(size_frame, width=5)
        self.height_entry.pack(side=tk.LEFT, padx=1)
        row += 1

        ttk.Label(prop_inner, text="Scale:").grid(row=row, column=0, sticky=tk.W, padx=2, pady=1)
        scale_frame = ttk.Frame(prop_inner)
        scale_frame.grid(row=row, column=1, sticky=tk.W, padx=2, pady=1)
        ttk.Label(scale_frame, text="X:").pack(side=tk.LEFT)
        self.scale_x_entry = ttk.Entry(scale_frame, width=5)
        self.scale_x_entry.pack(side=tk.LEFT, padx=1)
        ttk.Label(scale_frame, text="Y:").pack(side=tk.LEFT)
        self.scale_y_entry = ttk.Entry(scale_frame, width=5)
        self.scale_y_entry.pack(side=tk.LEFT, padx=1)
        row += 1

        ttk.Label(prop_inner, text="Anchor:").grid(row=row, column=0, sticky=tk.W, padx=2, pady=1)
        self.anchor_combo = ttk.Combobox(prop_inner, values=GUIElement.ANCHORS, width=15)
        self.anchor_combo.grid(row=row, column=1, sticky=tk.W, padx=2, pady=1)
        row += 1

        ttk.Label(prop_inner, text="Filepath:").grid(row=row, column=0, sticky=tk.W, padx=2, pady=1)
        fp_frame = ttk.Frame(prop_inner)
        fp_frame.grid(row=row, column=1, sticky=tk.W+tk.E, padx=2, pady=1)
        self.filepath_entry = ttk.Entry(fp_frame, width=12)
        self.filepath_entry.pack(side=tk.LEFT, fill=tk.X, expand=True)
        ttk.Button(fp_frame, text="...", width=3, command=self.browse_filepath).pack(side=tk.LEFT)
        row += 1

        ttk.Label(prop_inner, text="Font Size:").grid(row=row, column=0, sticky=tk.W, padx=2, pady=1)
        self.fontsize_entry = ttk.Entry(prop_inner, width=8)
        self.fontsize_entry.grid(row=row, column=1, sticky=tk.W, padx=2, pady=1)
        row += 1

        ttk.Label(prop_inner, text="Color:").grid(row=row, column=0, sticky=tk.W, padx=2, pady=1)
        color_frame = ttk.Frame(prop_inner)
        color_frame.grid(row=row, column=1, sticky=tk.W, padx=2, pady=1)
        self.color_label = tk.Label(color_frame, text="  ", bg="white", relief=tk.SUNKEN, width=3)
        self.color_label.pack(side=tk.LEFT)
        ttk.Button(color_frame, text="Pick", command=self.pick_color, width=4).pack(side=tk.LEFT, padx=2)
        row += 1

        self.visible_var = tk.BooleanVar(value=True)
        ttk.Checkbutton(prop_inner, text="Visible", variable=self.visible_var).grid(
            row=row, column=0, columnspan=2, sticky=tk.W, padx=2, pady=1)
        row += 1

        prop_inner.columnconfigure(1, weight=1)

        ttk.Button(prop_inner, text="Apply Changes", command=self.apply_properties).grid(
            row=row, column=0, columnspan=2, pady=5)

    def create_status_bar(self):
        self.status_var = tk.StringVar(value="Ready - Open a project or create a new layout")
        status_bar = ttk.Label(self.root, textvariable=self.status_var, relief=tk.SUNKEN, anchor=tk.W)
        status_bar.pack(side=tk.BOTTOM, fill=tk.X)

    # ============== Project Operations ==============

    def normalize_path(self, path):
        return path.replace("\\", "/")

    def open_project(self):
        folder = filedialog.askdirectory(
            title="Select Game Root Folder",
            mustexist=True
        )
        if folder:
            gui_path = os.path.join(folder, "data", "gui")
            if not os.path.exists(gui_path):
                messagebox.showerror("Error", "No 'data/gui' folder found.\nThis doesn't appear to be a valid game project.")
                return

            self.game_root = folder
            self.project_path_var.set(self.normalize_path(folder))
            self.load_gui_definitions()
            self.status_var.set(f"Opened project: {self.normalize_path(folder)}")

    def load_gui_definitions(self):
        """Load image and text definitions from gui.dat or separate files."""
        self.image_defs.clear()
        self.text_defs.clear()
        self.gui_screens.clear()
        self.image_cache.clear()

        gui_path = os.path.join(self.game_root, "data", "gui", "gui.dat")
        texts_path = os.path.join(self.game_root, "data", "gui", "texts.dat")
        images_path = os.path.join(self.game_root, "data", "gui", "images.dat")

        # Check for merged format
        use_merged = False
        if os.path.exists(gui_path):
            try:
                with open(gui_path, 'r') as f:
                    content = f.read()
                    if "*elements*" in content:
                        use_merged = True
            except:
                pass

        if use_merged:
            self._load_merged_gui(gui_path)
        else:
            self._load_separate_gui(gui_path, texts_path, images_path)

        self.refresh_screen_list()

    def _load_merged_gui(self, gui_path):
        """Load merged gui.dat format."""
        current_section = ""
        current_screen = ""

        try:
            with open(gui_path, 'r') as f:
                for line in f:
                    line = line.strip()
                    if line == "*elements*":
                        current_section = "elements"
                    elif line == "*images*":
                        current_section = "images"
                    elif line == "*texts*":
                        current_section = "texts"
                    elif line:
                        if current_section == "elements":
                            if line.startswith('*'):
                                current_screen = line[1:]
                                self.gui_screens[current_screen] = []
                            elif current_screen:
                                elem = self._parse_gui_element(line)
                                if elem:
                                    self.gui_screens[current_screen].append(elem)
                        elif current_section == "images":
                            parts = line.split('`')
                            if len(parts) >= 2:
                                self.image_defs[parts[0]] = parts[1]
                        elif current_section == "texts":
                            parts = line.split('`')
                            if len(parts) >= 2:
                                self.text_defs[parts[0]] = parts[1]
        except Exception as e:
            messagebox.showerror("Error", f"Failed to load gui.dat:\n{e}")

    def _load_separate_gui(self, gui_path, texts_path, images_path):
        """Load separate gui files (old format)."""
        # Load texts
        if os.path.exists(texts_path):
            try:
                with open(texts_path, 'r') as f:
                    for line in f:
                        line = line.strip()
                        if line:
                            parts = line.split('`')
                            if len(parts) >= 2:
                                self.text_defs[parts[0]] = parts[1]
            except Exception as e:
                print(f"Failed to load texts.dat: {e}")

        # Load images
        if os.path.exists(images_path):
            try:
                with open(images_path, 'r') as f:
                    for line in f:
                        line = line.strip()
                        if line:
                            parts = line.split('`')
                            if len(parts) >= 2:
                                self.image_defs[parts[0]] = parts[1]
            except Exception as e:
                print(f"Failed to load images.dat: {e}")

        # Load gui elements
        if os.path.exists(gui_path):
            current_screen = ""
            try:
                with open(gui_path, 'r') as f:
                    for line in f:
                        line = line.strip()
                        if line:
                            if line.startswith('*'):
                                current_screen = line[1:]
                                self.gui_screens[current_screen] = []
                            elif current_screen:
                                elem = self._parse_gui_element(line)
                                if elem:
                                    self.gui_screens[current_screen].append(elem)
            except Exception as e:
                print(f"Failed to load gui.dat: {e}")

    def _parse_gui_element(self, line):
        """Parse a GUI element line from gui.dat."""
        parts = line.split('`')
        name = parts[0]

        # Determine type based on definitions
        if name in self.image_defs:
            elem_type = "image"
            filepath = self.image_defs[name]
        elif name in self.text_defs:
            elem_type = "text"
            filepath = ""
        else:
            elem_type = "text"  # Default to text
            filepath = ""

        elem = GUIElement(elem_type, name, 0, 0)
        elem.filepath = filepath
        if name in self.text_defs:
            elem.text = self.text_defs[name]

        # Parse properties
        i = 1
        while i < len(parts) - 1:
            key = parts[i]
            val = parts[i + 1] if i + 1 < len(parts) else ""
            if key == 'pos':
                coords = val.split(',')
                if len(coords) >= 2:
                    elem.x = int(coords[0])
                    elem.y = int(coords[1])
                    if len(coords) >= 3:
                        elem.z_order = int(coords[2])
            elif key == 'scale':
                coords = val.split(',')
                if len(coords) >= 2:
                    elem.scale = (float(coords[0]), float(coords[1]))
                    elem.width = int(100 * float(coords[0]))
                    elem.height = int(100 * float(coords[1]))
            i += 2

        return elem

    # ============== Canvas Operations ==============

    def refresh_canvas(self):
        self.canvas.delete("all")

        # Draw anchor guides
        if self.show_guides.get():
            cx, cy = self.canvas_width // 2, self.canvas_height // 2
            # Center lines
            self.canvas.create_line(cx, 0, cx, self.canvas_height, fill="#444444", dash=(4, 4))
            self.canvas.create_line(0, cy, self.canvas_width, cy, fill="#444444", dash=(4, 4))
            # Border
            self.canvas.create_rectangle(2, 2, self.canvas_width - 2, self.canvas_height - 2,
                                        outline="#555555", width=1)

        # Draw elements sorted by layer
        for elem in sorted(self.elements, key=lambda e: e.layer):
            ax, ay = elem.get_anchor_offset(self.canvas_width, self.canvas_height)
            x = ax + elem.x
            y = ay + elem.y

            # Try to draw actual image
            if elem.elem_type == "image" and self.show_images.get() and HAS_PIL and elem.filepath:
                img = self.get_cached_image(elem.filepath, elem.width, elem.height)
                if img:
                    self.canvas.create_image(x, y, anchor=tk.NW, image=img)
                    outline = "yellow" if elem == self.selected_element else "white"
                    width = 2 if elem == self.selected_element else 1
                    self.canvas.create_rectangle(x, y, x + elem.width, y + elem.height,
                                                outline=outline, width=width)
                    continue

            # Colors based on type
            if elem.elem_type == "text":
                fill = "#44ff44"
            elif elem.elem_type == "image":
                fill = "#ff8844"
            elif elem.elem_type == "bar":
                fill = "#ff4444"
            else:  # rect
                fill = "#4488ff"

            outline = "yellow" if elem == self.selected_element else "white"
            width = 3 if elem == self.selected_element else 1

            self.canvas.create_rectangle(
                x, y, x + elem.width, y + elem.height,
                fill=fill, outline=outline, width=width
            )
            self.canvas.create_text(
                x + elem.width // 2, y + elem.height // 2,
                text=elem.text[:15] if elem.text else elem.name[:15], fill="white"
            )

        # Draw anchor indicator for selected
        if self.selected_element:
            ax, ay = self.selected_element.get_anchor_offset(self.canvas_width, self.canvas_height)
            self.canvas.create_oval(ax - 4, ay - 4, ax + 4, ay + 4, fill="red", outline="white")

    def get_cached_image(self, filepath, width, height):
        """Get cached image, loading if necessary."""
        if not self.game_root:
            return None

        cache_key = f"{filepath}_{width}_{height}"
        if cache_key in self.image_cache:
            return self.image_cache[cache_key]

        full_path = os.path.join(self.game_root, filepath)
        if not os.path.exists(full_path):
            return None

        try:
            img = Image.open(full_path).convert("RGBA")
            if width > 0 and height > 0:
                img = img.resize((width, height), Image.Resampling.LANCZOS)
            photo = ImageTk.PhotoImage(img)
            self.image_cache[cache_key] = photo
            return photo
        except Exception as e:
            print(f"Failed to load image {filepath}: {e}")
            return None

    def refresh_list(self):
        self.elem_listbox.delete(0, tk.END)
        for elem in sorted(self.elements, key=lambda e: e.layer):
            prefix = {"text": "[T]", "image": "[I]", "bar": "[B]", "rect": "[R]"}.get(elem.elem_type, "[?]")
            self.elem_listbox.insert(tk.END, f"{prefix} {elem.name}")

    def refresh_screen_list(self):
        self.screen_listbox.delete(0, tk.END)
        for name in sorted(self.gui_screens.keys()):
            self.screen_listbox.insert(tk.END, name)

    # ============== Screen Operations ==============

    def on_screen_selected(self, event=None):
        selection = self.screen_listbox.curselection()
        if selection:
            screen_name = self.screen_listbox.get(selection[0])
            self.current_screen = screen_name
            self.screen_name_var.set(screen_name)
            self.elements = self.gui_screens.get(screen_name, [])
            self.selected_element = None
            self.refresh_canvas()
            self.refresh_list()

    def add_screen(self):
        name = simpledialog.askstring("Add Screen", "Enter screen name:")
        if name:
            if name in self.gui_screens:
                messagebox.showwarning("Warning", f"Screen '{name}' already exists")
                return
            self.gui_screens[name] = []
            self.refresh_screen_list()
            self.modified = True
            self.update_title()

    def delete_screen(self):
        selection = self.screen_listbox.curselection()
        if selection:
            name = self.screen_listbox.get(selection[0])
            if messagebox.askyesno("Confirm", f"Delete screen '{name}'?"):
                del self.gui_screens[name]
                if self.current_screen == name:
                    self.current_screen = None
                    self.screen_name_var.set("(none)")
                    self.elements = []
                self.refresh_screen_list()
                self.refresh_canvas()
                self.refresh_list()
                self.modified = True
                self.update_title()

    # ============== Element Operations ==============

    def add_element(self, elem_type):
        name = simpledialog.askstring("Add Element", f"Enter {elem_type} name:")
        if name:
            if any(e.name == name for e in self.elements):
                messagebox.showwarning("Warning", f"Element '{name}' already exists")
                return

            elem = GUIElement(elem_type, name, 10, 10)
            elem.layer = len(self.elements)

            if elem_type == "text":
                elem.width = 150
                elem.height = 30
            elif elem_type == "image":
                elem.width = 64
                elem.height = 64
            elif elem_type == "bar":
                elem.width = 200
                elem.height = 20
            else:  # rect
                elem.width = 100
                elem.height = 100

            self.elements.append(elem)

            # Update gui_screens if we have a current screen
            if self.current_screen:
                self.gui_screens[self.current_screen] = self.elements

            self.refresh_canvas()
            self.refresh_list()
            self.modified = True
            self.update_title()

    def on_canvas_click(self, event):
        x = self.canvas.canvasx(event.x)
        y = self.canvas.canvasy(event.y)

        self.selected_element = None
        for elem in reversed(sorted(self.elements, key=lambda e: e.layer)):
            ax, ay = elem.get_anchor_offset(self.canvas_width, self.canvas_height)
            ex = ax + elem.x
            ey = ay + elem.y
            if ex <= x <= ex + elem.width and ey <= y <= ey + elem.height:
                self.selected_element = elem
                self.drag_data["x"] = x - ex
                self.drag_data["y"] = y - ey
                break

        self.refresh_canvas()
        self.show_properties()

        if self.selected_element:
            sorted_elems = sorted(self.elements, key=lambda e: e.layer)
            for i, elem in enumerate(sorted_elems):
                if elem == self.selected_element:
                    self.elem_listbox.selection_clear(0, tk.END)
                    self.elem_listbox.selection_set(i)
                    break

    def on_canvas_drag(self, event):
        if self.selected_element:
            x = self.canvas.canvasx(event.x) - self.drag_data["x"]
            y = self.canvas.canvasy(event.y) - self.drag_data["y"]

            ax, ay = self.selected_element.get_anchor_offset(self.canvas_width, self.canvas_height)

            # Snap to grid (5 pixels)
            new_x = round((x - ax) / 5) * 5
            new_y = round((y - ay) / 5) * 5

            self.selected_element.x = new_x
            self.selected_element.y = new_y

            self.refresh_canvas()
            self.show_properties()
            self.modified = True
            self.update_title()

    def on_canvas_release(self, event):
        pass

    def on_list_select(self, event):
        selection = self.elem_listbox.curselection()
        if selection:
            sorted_elems = sorted(self.elements, key=lambda e: e.layer)
            self.selected_element = sorted_elems[selection[0]]
            self.refresh_canvas()
            self.show_properties()

    def show_properties(self):
        if self.selected_element:
            elem = self.selected_element
            self.name_entry.delete(0, tk.END)
            self.name_entry.insert(0, elem.name)
            self.type_label.configure(text=elem.elem_type)
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
            self.scale_x_entry.delete(0, tk.END)
            self.scale_x_entry.insert(0, str(elem.scale[0]))
            self.scale_y_entry.delete(0, tk.END)
            self.scale_y_entry.insert(0, str(elem.scale[1]))
            self.anchor_combo.set(elem.anchor)
            self.filepath_entry.delete(0, tk.END)
            self.filepath_entry.insert(0, elem.filepath)
            self.fontsize_entry.delete(0, tk.END)
            self.fontsize_entry.insert(0, str(elem.font_size))
            self.visible_var.set(elem.visible)

            # Update color label
            r, g, b, a = elem.color
            hex_color = f"#{r:02x}{g:02x}{b:02x}"
            self.color_label.configure(bg=hex_color)

    def apply_properties(self):
        if not self.selected_element:
            return

        elem = self.selected_element
        new_name = self.name_entry.get().strip()

        if new_name != elem.name and any(e.name == new_name for e in self.elements):
            messagebox.showwarning("Warning", f"Element '{new_name}' already exists")
            return

        elem.name = new_name
        elem.text = self.text_entry.get()
        elem.x = int(self.x_entry.get() or 0)
        elem.y = int(self.y_entry.get() or 0)
        elem.width = int(self.width_entry.get() or 100)
        elem.height = int(self.height_entry.get() or 30)
        try:
            elem.scale = (float(self.scale_x_entry.get() or 1), float(self.scale_y_entry.get() or 1))
        except ValueError:
            elem.scale = (1.0, 1.0)
        elem.anchor = self.anchor_combo.get()
        elem.filepath = self.filepath_entry.get()
        elem.font_size = int(self.fontsize_entry.get() or 24)
        elem.visible = self.visible_var.get()

        self.refresh_canvas()
        self.refresh_list()
        self.modified = True
        self.update_title()

    def browse_filepath(self):
        if not self.game_root:
            messagebox.showwarning("Warning", "Open a project first")
            return

        filepath = filedialog.askopenfilename(
            title="Select Image",
            initialdir=os.path.join(self.game_root, "assets"),
            filetypes=[("Image files", "*.png *.jpg *.jpeg *.bmp"), ("All files", "*.*")]
        )
        if filepath:
            if filepath.startswith(self.game_root):
                filepath = filepath[len(self.game_root):].lstrip("/\\")
            self.filepath_entry.delete(0, tk.END)
            self.filepath_entry.insert(0, self.normalize_path(filepath))

    def pick_color(self):
        if not self.selected_element:
            return
        r, g, b, a = self.selected_element.color
        initial = f"#{r:02x}{g:02x}{b:02x}"
        color = colorchooser.askcolor(color=initial, title="Pick Color")
        if color[0]:
            r, g, b = [int(c) for c in color[0]]
            self.selected_element.color = (r, g, b, a)
            hex_color = f"#{r:02x}{g:02x}{b:02x}"
            self.color_label.configure(bg=hex_color)
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

    def duplicate_selected(self):
        if self.selected_element:
            new_name = f"{self.selected_element.name}_copy"
            i = 1
            while any(e.name == new_name for e in self.elements):
                new_name = f"{self.selected_element.name}_copy{i}"
                i += 1

            new_elem = GUIElement(
                self.selected_element.elem_type,
                new_name,
                self.selected_element.x + 20,
                self.selected_element.y + 20,
                self.selected_element.width,
                self.selected_element.height
            )
            new_elem.text = self.selected_element.text
            new_elem.filepath = self.selected_element.filepath
            new_elem.font_size = self.selected_element.font_size
            new_elem.color = self.selected_element.color
            new_elem.anchor = self.selected_element.anchor
            new_elem.scale = self.selected_element.scale
            new_elem.layer = len(self.elements)

            self.elements.append(new_elem)
            self.selected_element = new_elem
            self.refresh_canvas()
            self.refresh_list()
            self.modified = True
            self.update_title()

    def move_layer_up(self):
        if self.selected_element and self.selected_element.layer < len(self.elements) - 1:
            for elem in self.elements:
                if elem.layer == self.selected_element.layer + 1:
                    elem.layer -= 1
                    break
            self.selected_element.layer += 1
            self.refresh_canvas()
            self.refresh_list()
            self.modified = True
            self.update_title()

    def move_layer_down(self):
        if self.selected_element and self.selected_element.layer > 0:
            for elem in self.elements:
                if elem.layer == self.selected_element.layer - 1:
                    elem.layer += 1
                    break
            self.selected_element.layer -= 1
            self.refresh_canvas()
            self.refresh_list()
            self.modified = True
            self.update_title()

    # ============== File Operations ==============

    def edit_layout_settings(self):
        name = simpledialog.askstring("Layout Settings", "Layout name:", initialvalue=self.layout_name)
        if name:
            self.layout_name = name
            self.modified = True
            self.update_title()

    def edit_canvas_size(self):
        dialog = tk.Toplevel(self.root)
        dialog.title("Canvas Size")
        dialog.geometry("250x150")
        dialog.transient(self.root)
        dialog.grab_set()

        ttk.Label(dialog, text="Common sizes:").grid(row=0, column=0, columnspan=2, padx=5, pady=5)

        sizes = ["1280x720", "1920x1080", "800x600", "640x480"]
        size_var = tk.StringVar(value=f"{self.canvas_width}x{self.canvas_height}")
        size_combo = ttk.Combobox(dialog, textvariable=size_var, values=sizes, width=15)
        size_combo.grid(row=1, column=0, columnspan=2, padx=5, pady=5)

        ttk.Label(dialog, text="Width:").grid(row=2, column=0, padx=5, pady=5)
        width_entry = ttk.Entry(dialog, width=10)
        width_entry.insert(0, str(self.canvas_width))
        width_entry.grid(row=2, column=1, padx=5, pady=5)

        ttk.Label(dialog, text="Height:").grid(row=3, column=0, padx=5, pady=5)
        height_entry = ttk.Entry(dialog, width=10)
        height_entry.insert(0, str(self.canvas_height))
        height_entry.grid(row=3, column=1, padx=5, pady=5)

        def on_combo_change(event=None):
            try:
                w, h = size_var.get().split('x')
                width_entry.delete(0, tk.END)
                width_entry.insert(0, w)
                height_entry.delete(0, tk.END)
                height_entry.insert(0, h)
            except:
                pass

        size_combo.bind("<<ComboboxSelected>>", on_combo_change)

        def apply():
            self.canvas_width = int(width_entry.get())
            self.canvas_height = int(height_entry.get())
            self.canvas.configure(scrollregion=(0, 0, self.canvas_width, self.canvas_height))
            self.refresh_canvas()
            dialog.destroy()

        ttk.Button(dialog, text="Apply", command=apply).grid(row=4, column=0, columnspan=2, pady=10)

    def new_layout(self):
        if self.modified and not self.confirm_discard():
            return
        self.current_file = None
        self.layout_name = "GameGUI"
        self.elements = []
        self.selected_element = None
        self.current_screen = None
        self.screen_name_var.set("(none)")
        self.refresh_canvas()
        self.refresh_list()
        self.modified = False
        self.update_title()

    def open_file(self):
        if self.modified and not self.confirm_discard():
            return
        filepath = filedialog.askopenfilename(
            title="Open GUI Layout",
            filetypes=[("JSON files", "*.json"), ("All files", "*.*")]
        )
        if filepath:
            self.load_file(filepath)

    def load_file(self, filepath):
        try:
            with open(filepath, 'r') as f:
                data = json.load(f)
            self.layout_name = data.get("name", "GameGUI")
            self.canvas_width = data.get("width", 1280)
            self.canvas_height = data.get("height", 720)
            self.elements = [GUIElement.from_dict(e) for e in data.get("elements", [])]
            self.canvas.configure(scrollregion=(0, 0, self.canvas_width, self.canvas_height))
            self.current_file = filepath
            self.selected_element = None
            self.refresh_canvas()
            self.refresh_list()
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
        filepath = filedialog.asksaveasfilename(
            title="Save GUI Layout",
            filetypes=[("JSON files", "*.json"), ("All files", "*.*")],
            defaultextension=".json"
        )
        if filepath:
            self.write_file(filepath)
            self.current_file = filepath
            self.update_title()

    def write_file(self, filepath):
        try:
            data = {
                "name": self.layout_name,
                "width": self.canvas_width,
                "height": self.canvas_height,
                "elements": [e.to_dict() for e in self.elements]
            }
            with open(filepath, 'w') as f:
                json.dump(data, f, indent=2)
            self.modified = False
            self.update_title()
            self.status_var.set(f"Saved: {filepath}")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to save file:\n{e}")

    # ============== Import/Export gui.dat ==============

    def import_gui_dat(self):
        """Import from gui.dat into current screen."""
        if not self.game_root:
            messagebox.showwarning("Warning", "Open a project first")
            return

        if not self.current_screen:
            messagebox.showwarning("Warning", "Select or create a screen first")
            return

        # Elements should already be loaded when project was opened
        self.status_var.set(f"Loaded {len(self.elements)} elements from gui.dat")

    def export_gui_dat(self):
        """Export current screens to gui.dat format."""
        if not self.game_root:
            messagebox.showwarning("Warning", "Open a project first")
            return

        gui_path = os.path.join(self.game_root, "data", "gui", "gui.dat")

        try:
            with open(gui_path, 'w') as f:
                # Elements section
                f.write("*elements*\n")
                for screen_name, elements in self.gui_screens.items():
                    f.write(f"*{screen_name}\n")
                    for elem in elements:
                        line = elem.name
                        if elem.scale != (1.0, 1.0):
                            line += f"`scale`{int(elem.scale[0])},{int(elem.scale[1])}"
                        if elem.x != 0 or elem.y != 0:
                            line += f"`pos`{elem.x},{elem.y},{elem.z_order}"
                        f.write(line + "\n")
                    f.write("\n")

                # Images section
                f.write("*images*\n")
                for name, filepath in self.image_defs.items():
                    f.write(f"{name}`{filepath}\n")

                f.write("\n*texts*\n")
                for name, text in self.text_defs.items():
                    f.write(f"{name}`{text}\n")

            self.status_var.set(f"Exported to {gui_path}")
            messagebox.showinfo("Export Complete", f"GUI data exported to:\n{gui_path}")

        except Exception as e:
            messagebox.showerror("Error", f"Failed to export gui.dat:\n{e}")

    def export_cpp(self):
        filepath = filedialog.asksaveasfilename(
            title="Export C++ Code",
            filetypes=[("C++ files", "*.cpp"), ("Text files", "*.txt")],
            defaultextension=".cpp"
        )
        if not filepath:
            return

        lines = []
        lines.append(f"// Generated GUI code for: {self.layout_name}")
        lines.append(f"// Screen size: {self.canvas_width}x{self.canvas_height}")
        lines.append("")
        lines.append(f"void MyGUI::LoadData(Game* game)")
        lines.append("{")
        lines.append("    GUI::LoadData(game);")
        lines.append("")
        lines.append("    // Screen dimensions for anchor calculations")
        lines.append(f"    int screenW = game->screenWidth;")
        lines.append(f"    int screenH = game->screenHeight;")
        lines.append("")

        for elem in sorted(self.elements, key=lambda e: e.layer):
            if elem.elem_type == "text":
                anchor_code = self.get_anchor_code(elem)
                lines.append(f"    // {elem.name}")
                lines.append(f"    Text* {elem.name} = new Text(game->theFont, \"{elem.text}\");")
                lines.append(f"    {elem.name}->SetPosition({anchor_code});")
                lines.append(f"    texts.push_back({elem.name});")
                lines.append(f"    textNames[\"{elem.name}\"] = texts.size() - 1;")
                lines.append("")

            elif elem.elem_type == "image":
                anchor_code = self.get_anchor_code(elem)
                lines.append(f"    // {elem.name}")
                lines.append(f"    Entity* {elem.name} = new Entity(glm::vec3({anchor_code}, 0));")
                if elem.filepath:
                    lines.append(f"    {elem.name}->GetSprite()->SetTexture(game->spriteManager.GetImage(\"{elem.filepath}\"));")
                lines.append(f"    {elem.name}->GetSprite()->SetShader(game->renderer.shaders[0]);")
                lines.append(f"    {elem.name}->GetSprite()->keepPositionRelativeToCamera = true;")
                lines.append(f"    images.push_back({elem.name});")
                lines.append("")

            elif elem.elem_type == "bar":
                anchor_code = self.get_anchor_code(elem)
                lines.append(f"    // {elem.name} - Progress/Health bar")
                lines.append(f"    // Position: {anchor_code}")
                lines.append(f"    // Size: {elem.width}x{elem.height}")
                lines.append("")

            elif elem.elem_type == "rect":
                anchor_code = self.get_anchor_code(elem)
                r, g, b, a = elem.color
                lines.append(f"    // {elem.name} - Rectangle")
                lines.append(f"    // Position: {anchor_code}")
                lines.append(f"    // Size: {elem.width}x{elem.height}")
                lines.append(f"    // Color: rgba({r}, {g}, {b}, {a})")
                lines.append("")

        lines.append("}")

        try:
            with open(filepath, 'w') as f:
                f.write('\n'.join(lines))
            self.status_var.set(f"Exported: {filepath}")
            messagebox.showinfo("Export Complete", f"C++ code exported to:\n{filepath}")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to export:\n{e}")

    def get_anchor_code(self, elem):
        """Generate C++ code for position based on anchor"""
        x, y = elem.x, elem.y

        if elem.anchor == "top-left":
            return f"{x}, {y}"
        elif elem.anchor == "top-center":
            return f"screenW/2 + {x}, {y}"
        elif elem.anchor == "top-right":
            return f"screenW + {x}, {y}"
        elif elem.anchor == "center-left":
            return f"{x}, screenH/2 + {y}"
        elif elem.anchor == "center":
            return f"screenW/2 + {x}, screenH/2 + {y}"
        elif elem.anchor == "center-right":
            return f"screenW + {x}, screenH/2 + {y}"
        elif elem.anchor == "bottom-left":
            return f"{x}, screenH + {y}"
        elif elem.anchor == "bottom-center":
            return f"screenW/2 + {x}, screenH + {y}"
        elif elem.anchor == "bottom-right":
            return f"screenW + {x}, screenH + {y}"
        return f"{x}, {y}"

    def confirm_discard(self):
        return messagebox.askyesno("Unsaved Changes", "You have unsaved changes. Discard them?")

    def update_title(self):
        title = f"GUI Editor - {self.layout_name}"
        if self.current_file:
            title += f" ({os.path.basename(self.current_file)})"
        if self.modified:
            title += " *"
        self.root.title(title)

    def on_close(self):
        if self.modified and not self.confirm_discard():
            return
        self.root.destroy()


def main():
    root = tk.Tk()
    app = GUIEditor(root)
    root.protocol("WM_DELETE_WINDOW", app.on_close)
    root.mainloop()


if __name__ == "__main__":
    main()
