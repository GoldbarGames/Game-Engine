"""
Config Editor - GUI for editing game configuration files
Used for files in: data/config/ and data/gui/

Supports:
- input.config (merged keyboard/controller) or keyboard.config + controller.config
- gui.dat (merged or separate texts.dat, images.dat)
- istates.dat (initial states)

Format detection is automatic - supports both old and new file formats.
"""

import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import os


class ConfigEditor:
    def __init__(self, root):
        self.root = root
        self.root.title("Config Editor")
        self.root.geometry("900x650")

        # Project data
        self.game_root = None
        self.modified = False

        # Config data
        self.keyboard_mappings = {}  # name -> scancode
        self.controller_mappings = {}  # name -> button code
        self.gui_elements = {}  # gui_name -> list of {name, type, pos, scale}
        self.gui_images = {}  # name -> filepath
        self.gui_texts = {}  # name -> text
        self.initial_states = {}  # key -> value
        self.bgm_mappings = {}  # name -> filepath
        self.shaders = []  # list of {name, vertex, fragment}
        self.renderer_settings = {}  # key -> value
        self.settings_names = {}  # setting -> options string

        # Format flags
        self.use_merged_input = False
        self.use_merged_gui = False

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
        file_menu.add_command(label="Save All", command=self.save_all, accelerator="Ctrl+S")
        file_menu.add_separator()
        file_menu.add_command(label="Exit", command=self.on_close)

        self.root.bind("<Control-o>", lambda e: self.open_project())
        self.root.bind("<Control-s>", lambda e: self.save_all())

    def create_toolbar(self):
        toolbar = ttk.Frame(self.root)
        toolbar.pack(fill=tk.X, padx=5, pady=5)

        ttk.Button(toolbar, text="Open Project", command=self.open_project).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="Save All", command=self.save_all).pack(side=tk.LEFT, padx=2)

        ttk.Separator(toolbar, orient=tk.VERTICAL).pack(side=tk.LEFT, fill=tk.Y, padx=10)

        # Project path label
        self.project_path_var = tk.StringVar(value="No project open")
        ttk.Label(toolbar, textvariable=self.project_path_var, foreground="gray").pack(side=tk.LEFT, padx=5)

    def create_notebook(self):
        self.notebook = ttk.Notebook(self.root)
        self.notebook.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        # Tab 1: Input Mappings
        self.input_frame = ttk.Frame(self.notebook)
        self.notebook.add(self.input_frame, text="Input Mappings")
        self.create_input_tab()

        # Tab 2: GUI Elements
        self.gui_frame = ttk.Frame(self.notebook)
        self.notebook.add(self.gui_frame, text="GUI Elements")
        self.create_gui_tab()

        # Tab 3: Initial States
        self.states_frame = ttk.Frame(self.notebook)
        self.notebook.add(self.states_frame, text="Initial States")
        self.create_states_tab()

        # Tab 4: BGM
        self.bgm_frame = ttk.Frame(self.notebook)
        self.notebook.add(self.bgm_frame, text="BGM")
        self.create_bgm_tab()

        # Tab 5: Shaders
        self.shaders_frame = ttk.Frame(self.notebook)
        self.notebook.add(self.shaders_frame, text="Shaders")
        self.create_shaders_tab()

        # Tab 6: Renderer
        self.renderer_frame = ttk.Frame(self.notebook)
        self.notebook.add(self.renderer_frame, text="Renderer")
        self.create_renderer_tab()

        # Tab 7: Settings Names
        self.settings_names_frame = ttk.Frame(self.notebook)
        self.notebook.add(self.settings_names_frame, text="Settings Names")
        self.create_settings_names_tab()

    def create_input_tab(self):
        # Split into keyboard and controller sections
        paned = ttk.PanedWindow(self.input_frame, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        # Keyboard section
        kb_frame = ttk.LabelFrame(paned, text="Keyboard Mappings")
        paned.add(kb_frame, weight=1)

        kb_btn_frame = ttk.Frame(kb_frame)
        kb_btn_frame.pack(fill=tk.X, padx=5, pady=5)
        ttk.Button(kb_btn_frame, text="Add", command=self.add_keyboard_mapping).pack(side=tk.LEFT, padx=2)
        ttk.Button(kb_btn_frame, text="Delete", command=self.delete_keyboard_mapping).pack(side=tk.LEFT, padx=2)

        kb_list_frame = ttk.Frame(kb_frame)
        kb_list_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        self.keyboard_tree = ttk.Treeview(kb_list_frame, columns=("scancode", "name"), show="headings", height=15)
        self.keyboard_tree.heading("scancode", text="Scancode")
        self.keyboard_tree.heading("name", text="Action Name")
        self.keyboard_tree.column("scancode", width=80)
        self.keyboard_tree.column("name", width=150)

        kb_scroll = ttk.Scrollbar(kb_list_frame, orient=tk.VERTICAL, command=self.keyboard_tree.yview)
        self.keyboard_tree.configure(yscrollcommand=kb_scroll.set)

        self.keyboard_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        kb_scroll.pack(side=tk.RIGHT, fill=tk.Y)

        self.keyboard_tree.bind("<Double-1>", self.edit_keyboard_mapping)

        # Controller section
        ctrl_frame = ttk.LabelFrame(paned, text="Controller Mappings")
        paned.add(ctrl_frame, weight=1)

        ctrl_btn_frame = ttk.Frame(ctrl_frame)
        ctrl_btn_frame.pack(fill=tk.X, padx=5, pady=5)
        ttk.Button(ctrl_btn_frame, text="Add", command=self.add_controller_mapping).pack(side=tk.LEFT, padx=2)
        ttk.Button(ctrl_btn_frame, text="Delete", command=self.delete_controller_mapping).pack(side=tk.LEFT, padx=2)

        ctrl_list_frame = ttk.Frame(ctrl_frame)
        ctrl_list_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        self.controller_tree = ttk.Treeview(ctrl_list_frame, columns=("button", "name"), show="headings", height=15)
        self.controller_tree.heading("button", text="Button")
        self.controller_tree.heading("name", text="Action Name")
        self.controller_tree.column("button", width=80)
        self.controller_tree.column("name", width=150)

        ctrl_scroll = ttk.Scrollbar(ctrl_list_frame, orient=tk.VERTICAL, command=self.controller_tree.yview)
        self.controller_tree.configure(yscrollcommand=ctrl_scroll.set)

        self.controller_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        ctrl_scroll.pack(side=tk.RIGHT, fill=tk.Y)

        self.controller_tree.bind("<Double-1>", self.edit_controller_mapping)

    def create_gui_tab(self):
        # Left: GUI names list, Right: Elements in selected GUI
        paned = ttk.PanedWindow(self.gui_frame, orient=tk.HORIZONTAL)
        paned.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        # Left side - GUI screens
        left_frame = ttk.LabelFrame(paned, text="GUI Screens")
        paned.add(left_frame, weight=1)

        gui_btn_frame = ttk.Frame(left_frame)
        gui_btn_frame.pack(fill=tk.X, padx=5, pady=5)
        ttk.Button(gui_btn_frame, text="Add Screen", command=self.add_gui_screen).pack(side=tk.LEFT, padx=2)
        ttk.Button(gui_btn_frame, text="Delete", command=self.delete_gui_screen).pack(side=tk.LEFT, padx=2)

        gui_list_frame = ttk.Frame(left_frame)
        gui_list_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        self.gui_listbox = tk.Listbox(gui_list_frame, font=("Consolas", 11), exportselection=False)
        gui_scroll = ttk.Scrollbar(gui_list_frame, orient=tk.VERTICAL, command=self.gui_listbox.yview)
        self.gui_listbox.configure(yscrollcommand=gui_scroll.set)

        self.gui_listbox.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        gui_scroll.pack(side=tk.RIGHT, fill=tk.Y)

        self.gui_listbox.bind("<<ListboxSelect>>", self.on_gui_screen_selected)

        # Right side - Elements in GUI
        right_frame = ttk.LabelFrame(paned, text="Elements in Screen")
        paned.add(right_frame, weight=2)

        elem_btn_frame = ttk.Frame(right_frame)
        elem_btn_frame.pack(fill=tk.X, padx=5, pady=5)
        ttk.Button(elem_btn_frame, text="Add Element", command=self.add_gui_element).pack(side=tk.LEFT, padx=2)
        ttk.Button(elem_btn_frame, text="Delete", command=self.delete_gui_element).pack(side=tk.LEFT, padx=2)

        elem_list_frame = ttk.Frame(right_frame)
        elem_list_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        self.element_tree = ttk.Treeview(elem_list_frame, columns=("name", "type", "pos", "scale"), show="headings")
        self.element_tree.heading("name", text="Name")
        self.element_tree.heading("type", text="Type")
        self.element_tree.heading("pos", text="Position")
        self.element_tree.heading("scale", text="Scale")
        self.element_tree.column("name", width=100)
        self.element_tree.column("type", width=60)
        self.element_tree.column("pos", width=120)
        self.element_tree.column("scale", width=80)

        elem_scroll = ttk.Scrollbar(elem_list_frame, orient=tk.VERTICAL, command=self.element_tree.yview)
        self.element_tree.configure(yscrollcommand=elem_scroll.set)

        self.element_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        elem_scroll.pack(side=tk.RIGHT, fill=tk.Y)

        self.element_tree.bind("<Double-1>", self.edit_gui_element)

        # Bottom: Images and Texts definitions
        bottom_frame = ttk.Frame(self.gui_frame)
        bottom_frame.pack(fill=tk.X, padx=5, pady=5)

        # Images
        img_frame = ttk.LabelFrame(bottom_frame, text="Image Definitions")
        img_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(0, 5))

        img_btn_frame = ttk.Frame(img_frame)
        img_btn_frame.pack(fill=tk.X, padx=5, pady=2)
        ttk.Button(img_btn_frame, text="Add", command=self.add_image_def).pack(side=tk.LEFT, padx=2)
        ttk.Button(img_btn_frame, text="Delete", command=self.delete_image_def).pack(side=tk.LEFT, padx=2)

        self.image_tree = ttk.Treeview(img_frame, columns=("name", "path"), show="headings", height=5)
        self.image_tree.heading("name", text="Name")
        self.image_tree.heading("path", text="File Path")
        self.image_tree.column("name", width=80)
        self.image_tree.column("path", width=200)
        self.image_tree.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        self.image_tree.bind("<Double-1>", self.edit_image_def)

        # Texts
        txt_frame = ttk.LabelFrame(bottom_frame, text="Text Definitions")
        txt_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(5, 0))

        txt_btn_frame = ttk.Frame(txt_frame)
        txt_btn_frame.pack(fill=tk.X, padx=5, pady=2)
        ttk.Button(txt_btn_frame, text="Add", command=self.add_text_def).pack(side=tk.LEFT, padx=2)
        ttk.Button(txt_btn_frame, text="Delete", command=self.delete_text_def).pack(side=tk.LEFT, padx=2)

        self.text_tree = ttk.Treeview(txt_frame, columns=("name", "text"), show="headings", height=5)
        self.text_tree.heading("name", text="Name")
        self.text_tree.heading("text", text="Text Content")
        self.text_tree.column("name", width=80)
        self.text_tree.column("text", width=200)
        self.text_tree.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)
        self.text_tree.bind("<Double-1>", self.edit_text_def)

    def create_states_tab(self):
        # Simple key-value editor
        main_frame = ttk.Frame(self.states_frame)
        main_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        btn_frame = ttk.Frame(main_frame)
        btn_frame.pack(fill=tk.X, pady=5)
        ttk.Button(btn_frame, text="Add", command=self.add_state).pack(side=tk.LEFT, padx=2)
        ttk.Button(btn_frame, text="Delete", command=self.delete_state).pack(side=tk.LEFT, padx=2)

        list_frame = ttk.Frame(main_frame)
        list_frame.pack(fill=tk.BOTH, expand=True)

        self.states_tree = ttk.Treeview(list_frame, columns=("key", "value"), show="headings")
        self.states_tree.heading("key", text="Entity Type")
        self.states_tree.heading("value", text="Initial State")
        self.states_tree.column("key", width=150)
        self.states_tree.column("value", width=200)

        states_scroll = ttk.Scrollbar(list_frame, orient=tk.VERTICAL, command=self.states_tree.yview)
        self.states_tree.configure(yscrollcommand=states_scroll.set)

        self.states_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        states_scroll.pack(side=tk.RIGHT, fill=tk.Y)

        self.states_tree.bind("<Double-1>", self.edit_state)

    def create_bgm_tab(self):
        """Create BGM mappings tab."""
        main_frame = ttk.Frame(self.bgm_frame)
        main_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        btn_frame = ttk.Frame(main_frame)
        btn_frame.pack(fill=tk.X, pady=5)
        ttk.Button(btn_frame, text="Add", command=self.add_bgm).pack(side=tk.LEFT, padx=2)
        ttk.Button(btn_frame, text="Delete", command=self.delete_bgm).pack(side=tk.LEFT, padx=2)

        list_frame = ttk.Frame(main_frame)
        list_frame.pack(fill=tk.BOTH, expand=True)

        self.bgm_tree = ttk.Treeview(list_frame, columns=("name", "path"), show="headings")
        self.bgm_tree.heading("name", text="Music Name")
        self.bgm_tree.heading("path", text="File Path")
        self.bgm_tree.column("name", width=150)
        self.bgm_tree.column("path", width=300)

        bgm_scroll = ttk.Scrollbar(list_frame, orient=tk.VERTICAL, command=self.bgm_tree.yview)
        self.bgm_tree.configure(yscrollcommand=bgm_scroll.set)

        self.bgm_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        bgm_scroll.pack(side=tk.RIGHT, fill=tk.Y)

        self.bgm_tree.bind("<Double-1>", self.edit_bgm)

    def create_shaders_tab(self):
        """Create shaders tab."""
        main_frame = ttk.Frame(self.shaders_frame)
        main_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        btn_frame = ttk.Frame(main_frame)
        btn_frame.pack(fill=tk.X, pady=5)
        ttk.Button(btn_frame, text="Add", command=self.add_shader).pack(side=tk.LEFT, padx=2)
        ttk.Button(btn_frame, text="Delete", command=self.delete_shader).pack(side=tk.LEFT, padx=2)
        ttk.Button(btn_frame, text="Move Up", command=self.move_shader_up).pack(side=tk.LEFT, padx=2)
        ttk.Button(btn_frame, text="Move Down", command=self.move_shader_down).pack(side=tk.LEFT, padx=2)

        list_frame = ttk.Frame(main_frame)
        list_frame.pack(fill=tk.BOTH, expand=True)

        self.shaders_tree = ttk.Treeview(list_frame, columns=("name", "vertex", "fragment"), show="headings")
        self.shaders_tree.heading("name", text="Shader Name")
        self.shaders_tree.heading("vertex", text="Vertex File")
        self.shaders_tree.heading("fragment", text="Fragment File")
        self.shaders_tree.column("name", width=120)
        self.shaders_tree.column("vertex", width=150)
        self.shaders_tree.column("fragment", width=150)

        shaders_scroll = ttk.Scrollbar(list_frame, orient=tk.VERTICAL, command=self.shaders_tree.yview)
        self.shaders_tree.configure(yscrollcommand=shaders_scroll.set)

        self.shaders_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        shaders_scroll.pack(side=tk.RIGHT, fill=tk.Y)

        self.shaders_tree.bind("<Double-1>", self.edit_shader)

    def create_renderer_tab(self):
        """Create renderer settings tab."""
        main_frame = ttk.Frame(self.renderer_frame)
        main_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        btn_frame = ttk.Frame(main_frame)
        btn_frame.pack(fill=tk.X, pady=5)
        ttk.Button(btn_frame, text="Add", command=self.add_renderer_setting).pack(side=tk.LEFT, padx=2)
        ttk.Button(btn_frame, text="Delete", command=self.delete_renderer_setting).pack(side=tk.LEFT, padx=2)

        list_frame = ttk.Frame(main_frame)
        list_frame.pack(fill=tk.BOTH, expand=True)

        self.renderer_tree = ttk.Treeview(list_frame, columns=("key", "value"), show="headings")
        self.renderer_tree.heading("key", text="Setting")
        self.renderer_tree.heading("value", text="Value")
        self.renderer_tree.column("key", width=200)
        self.renderer_tree.column("value", width=200)

        renderer_scroll = ttk.Scrollbar(list_frame, orient=tk.VERTICAL, command=self.renderer_tree.yview)
        self.renderer_tree.configure(yscrollcommand=renderer_scroll.set)

        self.renderer_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        renderer_scroll.pack(side=tk.RIGHT, fill=tk.Y)

        self.renderer_tree.bind("<Double-1>", self.edit_renderer_setting)

    def create_settings_names_tab(self):
        """Create settings names tab."""
        main_frame = ttk.Frame(self.settings_names_frame)
        main_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        btn_frame = ttk.Frame(main_frame)
        btn_frame.pack(fill=tk.X, pady=5)
        ttk.Button(btn_frame, text="Add", command=self.add_setting_name).pack(side=tk.LEFT, padx=2)
        ttk.Button(btn_frame, text="Delete", command=self.delete_setting_name).pack(side=tk.LEFT, padx=2)

        list_frame = ttk.Frame(main_frame)
        list_frame.pack(fill=tk.BOTH, expand=True)

        self.settings_names_tree = ttk.Treeview(list_frame, columns=("setting", "options"), show="headings")
        self.settings_names_tree.heading("setting", text="Setting Name")
        self.settings_names_tree.heading("options", text="Options")
        self.settings_names_tree.column("setting", width=150)
        self.settings_names_tree.column("options", width=400)

        settings_scroll = ttk.Scrollbar(list_frame, orient=tk.VERTICAL, command=self.settings_names_tree.yview)
        self.settings_names_tree.configure(yscrollcommand=settings_scroll.set)

        self.settings_names_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        settings_scroll.pack(side=tk.RIGHT, fill=tk.Y)

        self.settings_names_tree.bind("<Double-1>", self.edit_setting_name)

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
        folder = filedialog.askdirectory(
            title="Select Game Root Folder",
            mustexist=True
        )
        if folder:
            config_path = os.path.join(folder, "data", "config")
            if not os.path.exists(config_path):
                messagebox.showerror("Error", "No 'data/config' folder found.\nThis doesn't appear to be a valid game project.")
                return

            self.game_root = folder
            self.project_path_var.set(self.normalize_path(folder))
            self.load_all_config()
            self.status_var.set(f"Opened project: {self.normalize_path(folder)}")

    def load_all_config(self):
        """Load all configuration files."""
        self.load_input_config()
        self.load_gui_config()
        self.load_initial_states()
        self.load_bgm()
        self.load_shaders()
        self.load_renderer()
        self.load_settings_names()
        self.modified = False
        self.update_title()

    def load_input_config(self):
        """Load keyboard and controller mappings."""
        self.keyboard_mappings.clear()
        self.controller_mappings.clear()

        # Check for merged input.config
        merged_path = os.path.join(self.game_root, "data", "config", "input.config")
        kb_path = os.path.join(self.game_root, "data", "config", "keyboard.config")
        ctrl_path = os.path.join(self.game_root, "data", "config", "controller.config")

        if os.path.exists(merged_path):
            self.use_merged_input = True
            current_section = ""
            try:
                with open(merged_path, 'r') as f:
                    for line in f:
                        line = line.strip()
                        if line == "*keyboard*":
                            current_section = "keyboard"
                        elif line == "*controller*":
                            current_section = "controller"
                        elif line:
                            parts = line.split(' ', 1)
                            if len(parts) == 2:
                                code = int(parts[0])
                                name = parts[1]
                                if current_section == "keyboard":
                                    self.keyboard_mappings[name] = code
                                elif current_section == "controller":
                                    self.controller_mappings[name] = code
            except Exception as e:
                messagebox.showerror("Error", f"Failed to load input.config:\n{e}")
        else:
            self.use_merged_input = False
            # Load separate files
            if os.path.exists(kb_path):
                try:
                    with open(kb_path, 'r') as f:
                        for line in f:
                            line = line.strip()
                            if line:
                                parts = line.split(' ', 1)
                                if len(parts) == 2:
                                    self.keyboard_mappings[parts[1]] = int(parts[0])
                except Exception as e:
                    messagebox.showerror("Error", f"Failed to load keyboard.config:\n{e}")

            if os.path.exists(ctrl_path):
                try:
                    with open(ctrl_path, 'r') as f:
                        for line in f:
                            line = line.strip()
                            if line:
                                parts = line.split(' ', 1)
                                if len(parts) == 2:
                                    self.controller_mappings[parts[1]] = int(parts[0])
                except Exception as e:
                    messagebox.showerror("Error", f"Failed to load controller.config:\n{e}")

        self.refresh_keyboard_tree()
        self.refresh_controller_tree()

    def load_gui_config(self):
        """Load GUI configuration (elements, images, texts)."""
        self.gui_elements.clear()
        self.gui_images.clear()
        self.gui_texts.clear()

        gui_path = os.path.join(self.game_root, "data", "gui", "gui.dat")
        texts_path = os.path.join(self.game_root, "data", "gui", "texts.dat")
        images_path = os.path.join(self.game_root, "data", "gui", "images.dat")

        # Check for merged format
        self.use_merged_gui = False
        if os.path.exists(gui_path):
            try:
                with open(gui_path, 'r') as f:
                    content = f.read()
                    if "*elements*" in content:
                        self.use_merged_gui = True
            except:
                pass

        if self.use_merged_gui:
            self._load_merged_gui(gui_path)
        else:
            self._load_separate_gui(gui_path, texts_path, images_path)

        self.refresh_gui_listbox()
        self.refresh_image_tree()
        self.refresh_text_tree()

    def _load_merged_gui(self, gui_path):
        """Load merged gui.dat format."""
        current_section = ""
        current_gui = ""

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
                                current_gui = line[1:]
                                self.gui_elements[current_gui] = []
                            else:
                                elem = self._parse_gui_element(line)
                                if current_gui and elem:
                                    self.gui_elements[current_gui].append(elem)
                        elif current_section == "images":
                            parts = line.split('`')
                            if len(parts) >= 2:
                                self.gui_images[parts[0]] = parts[1]
                        elif current_section == "texts":
                            parts = line.split('`')
                            if len(parts) >= 2:
                                self.gui_texts[parts[0]] = parts[1]
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
                                self.gui_texts[parts[0]] = parts[1]
            except Exception as e:
                messagebox.showerror("Error", f"Failed to load texts.dat:\n{e}")

        # Load images
        if os.path.exists(images_path):
            try:
                with open(images_path, 'r') as f:
                    for line in f:
                        line = line.strip()
                        if line:
                            parts = line.split('`')
                            if len(parts) >= 2:
                                self.gui_images[parts[0]] = parts[1]
            except Exception as e:
                messagebox.showerror("Error", f"Failed to load images.dat:\n{e}")

        # Load gui elements
        if os.path.exists(gui_path):
            current_gui = ""
            try:
                with open(gui_path, 'r') as f:
                    for line in f:
                        line = line.strip()
                        if line:
                            if line.startswith('*'):
                                current_gui = line[1:]
                                self.gui_elements[current_gui] = []
                            else:
                                elem = self._parse_gui_element(line)
                                if current_gui and elem:
                                    self.gui_elements[current_gui].append(elem)
            except Exception as e:
                messagebox.showerror("Error", f"Failed to load gui.dat:\n{e}")

    def _parse_gui_element(self, line):
        """Parse a GUI element line."""
        parts = line.split('`')
        name = parts[0]
        elem = {
            'name': name,
            'type': 'text' if name in self.gui_texts or name not in self.gui_images else 'image',
            'pos': (0, 0, 0),
            'scale': (1, 1)
        }

        i = 1
        while i < len(parts) - 1:
            key = parts[i]
            val = parts[i + 1] if i + 1 < len(parts) else ""
            if key == 'pos':
                coords = val.split(',')
                if len(coords) >= 3:
                    elem['pos'] = (int(coords[0]), int(coords[1]), int(coords[2]))
                elif len(coords) >= 2:
                    elem['pos'] = (int(coords[0]), int(coords[1]), 0)
            elif key == 'scale':
                coords = val.split(',')
                if len(coords) >= 2:
                    elem['scale'] = (int(coords[0]), int(coords[1]))
            i += 2

        # Determine type based on whether it's in images or texts
        if name in self.gui_images:
            elem['type'] = 'image'
        elif name in self.gui_texts:
            elem['type'] = 'text'

        return elem

    def load_initial_states(self):
        """Load initial states from istates.dat."""
        self.initial_states.clear()
        states_path = os.path.join(self.game_root, "data", "config", "istates.dat")

        if os.path.exists(states_path):
            try:
                with open(states_path, 'r') as f:
                    for line in f:
                        line = line.strip()
                        if line:
                            parts = line.split(' ', 1)
                            if len(parts) == 2:
                                self.initial_states[parts[0]] = parts[1]
                            elif len(parts) == 1:
                                self.initial_states[parts[0]] = ""
            except Exception as e:
                messagebox.showerror("Error", f"Failed to load istates.dat:\n{e}")

        self.refresh_states_tree()

    def load_bgm(self):
        """Load BGM mappings from bgm.dat."""
        self.bgm_mappings.clear()
        bgm_path = os.path.join(self.game_root, "data", "config", "bgm.dat")

        if os.path.exists(bgm_path):
            try:
                with open(bgm_path, 'r') as f:
                    for line in f:
                        line = line.strip()
                        if line:
                            parts = line.split(' ', 1)
                            if len(parts) == 2:
                                self.bgm_mappings[parts[0]] = parts[1]
            except Exception as e:
                messagebox.showerror("Error", f"Failed to load bgm.dat:\n{e}")

        self.refresh_bgm_tree()

    def load_shaders(self):
        """Load shaders from shaders.dat."""
        self.shaders.clear()
        shaders_path = os.path.join(self.game_root, "data", "config", "shaders.dat")

        if os.path.exists(shaders_path):
            try:
                with open(shaders_path, 'r') as f:
                    for line in f:
                        line = line.strip()
                        if line:
                            parts = line.split()
                            if len(parts) >= 3:
                                self.shaders.append({
                                    'name': parts[0],
                                    'vertex': parts[1],
                                    'fragment': parts[2]
                                })
            except Exception as e:
                messagebox.showerror("Error", f"Failed to load shaders.dat:\n{e}")

        self.refresh_shaders_tree()

    def load_renderer(self):
        """Load renderer settings from renderer.dat."""
        self.renderer_settings.clear()
        renderer_path = os.path.join(self.game_root, "data", "config", "renderer.dat")

        if os.path.exists(renderer_path):
            try:
                with open(renderer_path, 'r') as f:
                    for line in f:
                        line = line.strip()
                        if line:
                            parts = line.split(' ', 1)
                            if len(parts) == 2:
                                self.renderer_settings[parts[0]] = parts[1]
                            elif len(parts) == 1:
                                self.renderer_settings[parts[0]] = ""
            except Exception as e:
                messagebox.showerror("Error", f"Failed to load renderer.dat:\n{e}")

        self.refresh_renderer_tree()

    def load_settings_names(self):
        """Load settings names from settings.names."""
        self.settings_names.clear()
        settings_path = os.path.join(self.game_root, "data", "config", "settings.names")

        if os.path.exists(settings_path):
            try:
                with open(settings_path, 'r') as f:
                    for line in f:
                        line = line.strip()
                        if line and ':' in line:
                            parts = line.split(':', 1)
                            self.settings_names[parts[0].strip()] = parts[1].strip()
            except Exception as e:
                messagebox.showerror("Error", f"Failed to load settings.names:\n{e}")

        self.refresh_settings_names_tree()

    # ============== Save Operations ==============

    def save_all(self):
        """Save all configuration files."""
        if not self.game_root:
            messagebox.showwarning("Warning", "No project open")
            return

        self.save_input_config()
        self.save_gui_config()
        self.save_initial_states()
        self.save_bgm()
        self.save_shaders()
        self.save_renderer()
        self.save_settings_names()

        self.modified = False
        self.update_title()
        self.status_var.set("All configuration files saved")

    def save_input_config(self):
        """Save keyboard and controller mappings."""
        if self.use_merged_input:
            path = os.path.join(self.game_root, "data", "config", "input.config")
            try:
                with open(path, 'w') as f:
                    f.write("*keyboard*\n")
                    for name, code in self.keyboard_mappings.items():
                        f.write(f"{code} {name}\n")
                    f.write("\n*controller*\n")
                    for name, code in self.controller_mappings.items():
                        f.write(f"{code} {name}\n")
            except Exception as e:
                messagebox.showerror("Error", f"Failed to save input.config:\n{e}")
        else:
            # Save separate files
            kb_path = os.path.join(self.game_root, "data", "config", "keyboard.config")
            ctrl_path = os.path.join(self.game_root, "data", "config", "controller.config")

            try:
                with open(kb_path, 'w') as f:
                    for name, code in self.keyboard_mappings.items():
                        f.write(f"{code} {name}\n")
            except Exception as e:
                messagebox.showerror("Error", f"Failed to save keyboard.config:\n{e}")

            try:
                with open(ctrl_path, 'w') as f:
                    for name, code in self.controller_mappings.items():
                        f.write(f"{code} {name}\n")
            except Exception as e:
                messagebox.showerror("Error", f"Failed to save controller.config:\n{e}")

    def save_gui_config(self):
        """Save GUI configuration."""
        if self.use_merged_gui:
            path = os.path.join(self.game_root, "data", "gui", "gui.dat")
            try:
                with open(path, 'w') as f:
                    # Elements section
                    f.write("*elements*\n")
                    for gui_name, elements in self.gui_elements.items():
                        f.write(f"*{gui_name}\n")
                        for elem in elements:
                            line = elem['name']
                            if elem['scale'] != (1, 1):
                                line += f"`scale`{elem['scale'][0]},{elem['scale'][1]}"
                            if elem['pos'] != (0, 0, 0):
                                line += f"`pos`{elem['pos'][0]},{elem['pos'][1]},{elem['pos'][2]}"
                            f.write(line + "\n")
                        f.write("\n")

                    # Images section
                    f.write("*images*\n")
                    for name, filepath in self.gui_images.items():
                        f.write(f"{name}`{filepath}\n")

                    f.write("\n*texts*\n")
                    for name, text in self.gui_texts.items():
                        f.write(f"{name}`{text}\n")
            except Exception as e:
                messagebox.showerror("Error", f"Failed to save gui.dat:\n{e}")
        else:
            # Save separate files
            gui_path = os.path.join(self.game_root, "data", "gui", "gui.dat")
            texts_path = os.path.join(self.game_root, "data", "gui", "texts.dat")
            images_path = os.path.join(self.game_root, "data", "gui", "images.dat")

            try:
                with open(texts_path, 'w') as f:
                    for name, text in self.gui_texts.items():
                        f.write(f"{name}`{text}\n")
            except Exception as e:
                messagebox.showerror("Error", f"Failed to save texts.dat:\n{e}")

            try:
                with open(images_path, 'w') as f:
                    for name, filepath in self.gui_images.items():
                        f.write(f"{name}`{filepath}\n")
            except Exception as e:
                messagebox.showerror("Error", f"Failed to save images.dat:\n{e}")

            try:
                with open(gui_path, 'w') as f:
                    for gui_name, elements in self.gui_elements.items():
                        f.write(f"*{gui_name}\n")
                        for elem in elements:
                            line = elem['name']
                            if elem['scale'] != (1, 1):
                                line += f"`scale`{elem['scale'][0]},{elem['scale'][1]}"
                            if elem['pos'] != (0, 0, 0):
                                line += f"`pos`{elem['pos'][0]},{elem['pos'][1]},{elem['pos'][2]}"
                            f.write(line + "\n")
            except Exception as e:
                messagebox.showerror("Error", f"Failed to save gui.dat:\n{e}")

    def save_initial_states(self):
        """Save initial states."""
        path = os.path.join(self.game_root, "data", "config", "istates.dat")
        try:
            with open(path, 'w') as f:
                for key, value in self.initial_states.items():
                    f.write(f"{key} {value}\n")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to save istates.dat:\n{e}")

    def save_bgm(self):
        """Save BGM mappings to bgm.dat."""
        path = os.path.join(self.game_root, "data", "config", "bgm.dat")
        try:
            with open(path, 'w') as f:
                for name, filepath in self.bgm_mappings.items():
                    f.write(f"{name} {filepath}\n")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to save bgm.dat:\n{e}")

    def save_shaders(self):
        """Save shaders to shaders.dat."""
        path = os.path.join(self.game_root, "data", "config", "shaders.dat")
        try:
            with open(path, 'w') as f:
                for shader in self.shaders:
                    f.write(f"{shader['name']} {shader['vertex']} {shader['fragment']}\n")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to save shaders.dat:\n{e}")

    def save_renderer(self):
        """Save renderer settings to renderer.dat."""
        path = os.path.join(self.game_root, "data", "config", "renderer.dat")
        try:
            with open(path, 'w') as f:
                for key, value in self.renderer_settings.items():
                    f.write(f"{key} {value}\n")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to save renderer.dat:\n{e}")

    def save_settings_names(self):
        """Save settings names to settings.names."""
        path = os.path.join(self.game_root, "data", "config", "settings.names")
        try:
            with open(path, 'w') as f:
                for setting, options in self.settings_names.items():
                    f.write(f"{setting}: {options}\n")
        except Exception as e:
            messagebox.showerror("Error", f"Failed to save settings.names:\n{e}")

    # ============== Refresh UI ==============

    def refresh_keyboard_tree(self):
        self.keyboard_tree.delete(*self.keyboard_tree.get_children())
        for name, code in sorted(self.keyboard_mappings.items(), key=lambda x: x[0]):
            self.keyboard_tree.insert("", tk.END, values=(code, name))

    def refresh_controller_tree(self):
        self.controller_tree.delete(*self.controller_tree.get_children())
        for name, code in sorted(self.controller_mappings.items(), key=lambda x: x[0]):
            self.controller_tree.insert("", tk.END, values=(code, name))

    def refresh_gui_listbox(self):
        self.gui_listbox.delete(0, tk.END)
        for name in sorted(self.gui_elements.keys()):
            self.gui_listbox.insert(tk.END, name)

    def refresh_element_tree(self):
        self.element_tree.delete(*self.element_tree.get_children())
        selection = self.gui_listbox.curselection()
        if selection:
            gui_name = self.gui_listbox.get(selection[0])
            if gui_name in self.gui_elements:
                for elem in self.gui_elements[gui_name]:
                    pos_str = f"{elem['pos'][0]},{elem['pos'][1]},{elem['pos'][2]}"
                    scale_str = f"{elem['scale'][0]},{elem['scale'][1]}"
                    self.element_tree.insert("", tk.END, values=(elem['name'], elem['type'], pos_str, scale_str))

    def refresh_image_tree(self):
        self.image_tree.delete(*self.image_tree.get_children())
        for name, path in sorted(self.gui_images.items()):
            self.image_tree.insert("", tk.END, values=(name, path))

    def refresh_text_tree(self):
        self.text_tree.delete(*self.text_tree.get_children())
        for name, text in sorted(self.gui_texts.items()):
            self.text_tree.insert("", tk.END, values=(name, text))

    def refresh_states_tree(self):
        self.states_tree.delete(*self.states_tree.get_children())
        for key, value in sorted(self.initial_states.items()):
            self.states_tree.insert("", tk.END, values=(key, value))

    def refresh_bgm_tree(self):
        self.bgm_tree.delete(*self.bgm_tree.get_children())
        for name, path in sorted(self.bgm_mappings.items()):
            self.bgm_tree.insert("", tk.END, values=(name, path))

    def refresh_shaders_tree(self):
        self.shaders_tree.delete(*self.shaders_tree.get_children())
        for shader in self.shaders:
            self.shaders_tree.insert("", tk.END, values=(shader['name'], shader['vertex'], shader['fragment']))

    def refresh_renderer_tree(self):
        self.renderer_tree.delete(*self.renderer_tree.get_children())
        for key, value in sorted(self.renderer_settings.items()):
            self.renderer_tree.insert("", tk.END, values=(key, value))

    def refresh_settings_names_tree(self):
        self.settings_names_tree.delete(*self.settings_names_tree.get_children())
        for setting, options in sorted(self.settings_names.items()):
            self.settings_names_tree.insert("", tk.END, values=(setting, options))

    def on_gui_screen_selected(self, event=None):
        self.refresh_element_tree()

    # ============== Input Mapping Operations ==============

    def add_keyboard_mapping(self):
        self._show_mapping_dialog("keyboard", None)

    def edit_keyboard_mapping(self, event=None):
        selection = self.keyboard_tree.selection()
        if selection:
            item = self.keyboard_tree.item(selection[0])
            name = item['values'][1]
            self._show_mapping_dialog("keyboard", name)

    def delete_keyboard_mapping(self):
        selection = self.keyboard_tree.selection()
        if selection:
            item = self.keyboard_tree.item(selection[0])
            name = item['values'][1]
            if messagebox.askyesno("Confirm", f"Delete mapping '{name}'?"):
                del self.keyboard_mappings[name]
                self.refresh_keyboard_tree()
                self.mark_modified()

    def add_controller_mapping(self):
        self._show_mapping_dialog("controller", None)

    def edit_controller_mapping(self, event=None):
        selection = self.controller_tree.selection()
        if selection:
            item = self.controller_tree.item(selection[0])
            name = item['values'][1]
            self._show_mapping_dialog("controller", name)

    def delete_controller_mapping(self):
        selection = self.controller_tree.selection()
        if selection:
            item = self.controller_tree.item(selection[0])
            name = item['values'][1]
            if messagebox.askyesno("Confirm", f"Delete mapping '{name}'?"):
                del self.controller_mappings[name]
                self.refresh_controller_tree()
                self.mark_modified()

    def _show_mapping_dialog(self, mapping_type, existing_name):
        dialog = tk.Toplevel(self.root)
        dialog.title("Edit Mapping" if existing_name else "Add Mapping")
        dialog.geometry("300x150")
        dialog.transient(self.root)
        dialog.grab_set()

        mappings = self.keyboard_mappings if mapping_type == "keyboard" else self.controller_mappings
        code_label = "Scancode:" if mapping_type == "keyboard" else "Button:"

        ttk.Label(dialog, text="Action Name:").pack(pady=(10, 5))
        name_entry = ttk.Entry(dialog, width=25)
        name_entry.pack(pady=5)

        ttk.Label(dialog, text=code_label).pack(pady=5)
        code_entry = ttk.Entry(dialog, width=25)
        code_entry.pack(pady=5)

        if existing_name:
            name_entry.insert(0, existing_name)
            code_entry.insert(0, str(mappings[existing_name]))

        def do_save():
            name = name_entry.get().strip()
            code_str = code_entry.get().strip()
            if not name or not code_str:
                return
            try:
                code = int(code_str)
            except ValueError:
                messagebox.showerror("Error", "Code must be a number")
                return

            if existing_name and existing_name != name:
                del mappings[existing_name]
            mappings[name] = code

            if mapping_type == "keyboard":
                self.refresh_keyboard_tree()
            else:
                self.refresh_controller_tree()

            self.mark_modified()
            dialog.destroy()

        ttk.Button(dialog, text="Save", command=do_save).pack(pady=10)

    # ============== GUI Operations ==============

    def add_gui_screen(self):
        dialog = tk.Toplevel(self.root)
        dialog.title("Add GUI Screen")
        dialog.geometry("300x100")
        dialog.transient(self.root)
        dialog.grab_set()

        ttk.Label(dialog, text="Screen Name:").pack(pady=(10, 5))
        name_entry = ttk.Entry(dialog, width=25)
        name_entry.pack(pady=5)
        name_entry.focus_set()

        def do_add():
            name = name_entry.get().strip()
            if not name:
                return
            if name in self.gui_elements:
                messagebox.showwarning("Warning", f"Screen '{name}' already exists")
                return
            self.gui_elements[name] = []
            self.refresh_gui_listbox()
            self.mark_modified()
            dialog.destroy()

        name_entry.bind("<Return>", lambda e: do_add())
        ttk.Button(dialog, text="Add", command=do_add).pack(pady=10)

    def delete_gui_screen(self):
        selection = self.gui_listbox.curselection()
        if selection:
            name = self.gui_listbox.get(selection[0])
            if messagebox.askyesno("Confirm", f"Delete screen '{name}'?"):
                del self.gui_elements[name]
                self.refresh_gui_listbox()
                self.refresh_element_tree()
                self.mark_modified()

    def add_gui_element(self):
        selection = self.gui_listbox.curselection()
        if not selection:
            messagebox.showwarning("Warning", "Select a GUI screen first")
            return
        gui_name = self.gui_listbox.get(selection[0])
        self._show_element_dialog(gui_name, None)

    def edit_gui_element(self, event=None):
        gui_selection = self.gui_listbox.curselection()
        elem_selection = self.element_tree.selection()
        if gui_selection and elem_selection:
            gui_name = self.gui_listbox.get(gui_selection[0])
            item = self.element_tree.item(elem_selection[0])
            elem_name = item['values'][0]
            self._show_element_dialog(gui_name, elem_name)

    def delete_gui_element(self):
        gui_selection = self.gui_listbox.curselection()
        elem_selection = self.element_tree.selection()
        if gui_selection and elem_selection:
            gui_name = self.gui_listbox.get(gui_selection[0])
            item = self.element_tree.item(elem_selection[0])
            elem_name = item['values'][0]
            if messagebox.askyesno("Confirm", f"Delete element '{elem_name}'?"):
                self.gui_elements[gui_name] = [e for e in self.gui_elements[gui_name] if e['name'] != elem_name]
                self.refresh_element_tree()
                self.mark_modified()

    def _show_element_dialog(self, gui_name, existing_name):
        dialog = tk.Toplevel(self.root)
        dialog.title("Edit Element" if existing_name else "Add Element")
        dialog.geometry("350x250")
        dialog.transient(self.root)
        dialog.grab_set()

        existing_elem = None
        if existing_name:
            for elem in self.gui_elements[gui_name]:
                if elem['name'] == existing_name:
                    existing_elem = elem
                    break

        # Available elements (images and texts)
        available = list(self.gui_images.keys()) + list(self.gui_texts.keys())

        ttk.Label(dialog, text="Element Name:").pack(pady=(10, 5))
        name_var = tk.StringVar()
        name_combo = ttk.Combobox(dialog, textvariable=name_var, values=available, width=25)
        name_combo.pack(pady=5)

        pos_frame = ttk.Frame(dialog)
        pos_frame.pack(pady=5)
        ttk.Label(pos_frame, text="Position (x,y,z):").pack(side=tk.LEFT)
        pos_entry = ttk.Entry(pos_frame, width=15)
        pos_entry.pack(side=tk.LEFT, padx=5)

        scale_frame = ttk.Frame(dialog)
        scale_frame.pack(pady=5)
        ttk.Label(scale_frame, text="Scale (x,y):").pack(side=tk.LEFT)
        scale_entry = ttk.Entry(scale_frame, width=15)
        scale_entry.pack(side=tk.LEFT, padx=5)

        if existing_elem:
            name_var.set(existing_elem['name'])
            pos_entry.insert(0, f"{existing_elem['pos'][0]},{existing_elem['pos'][1]},{existing_elem['pos'][2]}")
            scale_entry.insert(0, f"{existing_elem['scale'][0]},{existing_elem['scale'][1]}")
        else:
            pos_entry.insert(0, "0,0,0")
            scale_entry.insert(0, "1,1")

        def do_save():
            name = name_var.get().strip()
            if not name:
                return

            pos_str = pos_entry.get().strip()
            scale_str = scale_entry.get().strip()

            try:
                pos_parts = pos_str.split(',')
                pos = (int(pos_parts[0]), int(pos_parts[1]), int(pos_parts[2]) if len(pos_parts) > 2 else 0)
            except:
                pos = (0, 0, 0)

            try:
                scale_parts = scale_str.split(',')
                scale = (int(scale_parts[0]), int(scale_parts[1]))
            except:
                scale = (1, 1)

            elem_type = 'image' if name in self.gui_images else 'text'

            new_elem = {'name': name, 'type': elem_type, 'pos': pos, 'scale': scale}

            if existing_name:
                # Replace existing
                for i, e in enumerate(self.gui_elements[gui_name]):
                    if e['name'] == existing_name:
                        self.gui_elements[gui_name][i] = new_elem
                        break
            else:
                self.gui_elements[gui_name].append(new_elem)

            self.refresh_element_tree()
            self.mark_modified()
            dialog.destroy()

        ttk.Button(dialog, text="Save", command=do_save).pack(pady=10)

    def add_image_def(self):
        self._show_image_text_dialog("image", None)

    def edit_image_def(self, event=None):
        selection = self.image_tree.selection()
        if selection:
            item = self.image_tree.item(selection[0])
            name = item['values'][0]
            self._show_image_text_dialog("image", name)

    def delete_image_def(self):
        selection = self.image_tree.selection()
        if selection:
            item = self.image_tree.item(selection[0])
            name = item['values'][0]
            if messagebox.askyesno("Confirm", f"Delete image '{name}'?"):
                del self.gui_images[name]
                self.refresh_image_tree()
                self.mark_modified()

    def add_text_def(self):
        self._show_image_text_dialog("text", None)

    def edit_text_def(self, event=None):
        selection = self.text_tree.selection()
        if selection:
            item = self.text_tree.item(selection[0])
            name = item['values'][0]
            self._show_image_text_dialog("text", name)

    def delete_text_def(self):
        selection = self.text_tree.selection()
        if selection:
            item = self.text_tree.item(selection[0])
            name = item['values'][0]
            if messagebox.askyesno("Confirm", f"Delete text '{name}'?"):
                del self.gui_texts[name]
                self.refresh_text_tree()
                self.mark_modified()

    def _show_image_text_dialog(self, item_type, existing_name):
        dialog = tk.Toplevel(self.root)
        dialog.title(f"Edit {item_type.title()}" if existing_name else f"Add {item_type.title()}")
        dialog.geometry("400x150")
        dialog.transient(self.root)
        dialog.grab_set()

        data = self.gui_images if item_type == "image" else self.gui_texts
        value_label = "File Path:" if item_type == "image" else "Text Content:"

        ttk.Label(dialog, text="Name:").pack(pady=(10, 5))
        name_entry = ttk.Entry(dialog, width=35)
        name_entry.pack(pady=5)

        value_frame = ttk.Frame(dialog)
        value_frame.pack(pady=5, fill=tk.X, padx=10)
        ttk.Label(value_frame, text=value_label).pack(side=tk.LEFT)
        value_entry = ttk.Entry(value_frame, width=30)
        value_entry.pack(side=tk.LEFT, padx=5, expand=True, fill=tk.X)

        if item_type == "image":
            def browse():
                filepath = filedialog.askopenfilename(
                    title="Select Image",
                    initialdir=os.path.join(self.game_root, "assets") if self.game_root else None,
                    filetypes=[("Image files", "*.png *.jpg *.jpeg *.bmp"), ("All files", "*.*")]
                )
                if filepath:
                    # Make relative to game root
                    if self.game_root and filepath.startswith(self.game_root):
                        filepath = filepath[len(self.game_root):].lstrip("/\\")
                    value_entry.delete(0, tk.END)
                    value_entry.insert(0, self.normalize_path(filepath))

            ttk.Button(value_frame, text="Browse", command=browse).pack(side=tk.LEFT, padx=5)

        if existing_name:
            name_entry.insert(0, existing_name)
            value_entry.insert(0, data[existing_name])

        def do_save():
            name = name_entry.get().strip()
            value = value_entry.get().strip()
            if not name:
                return

            if existing_name and existing_name != name:
                del data[existing_name]
            data[name] = value

            if item_type == "image":
                self.refresh_image_tree()
            else:
                self.refresh_text_tree()

            self.mark_modified()
            dialog.destroy()

        ttk.Button(dialog, text="Save", command=do_save).pack(pady=10)

    # ============== States Operations ==============

    def add_state(self):
        self._show_state_dialog(None)

    def edit_state(self, event=None):
        selection = self.states_tree.selection()
        if selection:
            item = self.states_tree.item(selection[0])
            key = item['values'][0]
            self._show_state_dialog(key)

    def delete_state(self):
        selection = self.states_tree.selection()
        if selection:
            item = self.states_tree.item(selection[0])
            key = item['values'][0]
            if messagebox.askyesno("Confirm", f"Delete state '{key}'?"):
                del self.initial_states[key]
                self.refresh_states_tree()
                self.mark_modified()

    def _show_state_dialog(self, existing_key):
        dialog = tk.Toplevel(self.root)
        dialog.title("Edit State" if existing_key else "Add State")
        dialog.geometry("300x150")
        dialog.transient(self.root)
        dialog.grab_set()

        ttk.Label(dialog, text="Entity Type:").pack(pady=(10, 5))
        key_entry = ttk.Entry(dialog, width=25)
        key_entry.pack(pady=5)

        ttk.Label(dialog, text="Initial State:").pack(pady=5)
        value_entry = ttk.Entry(dialog, width=25)
        value_entry.pack(pady=5)

        if existing_key:
            key_entry.insert(0, existing_key)
            value_entry.insert(0, self.initial_states[existing_key])

        def do_save():
            key = key_entry.get().strip()
            value = value_entry.get().strip()
            if not key:
                return

            if existing_key and existing_key != key:
                del self.initial_states[existing_key]
            self.initial_states[key] = value

            self.refresh_states_tree()
            self.mark_modified()
            dialog.destroy()

        ttk.Button(dialog, text="Save", command=do_save).pack(pady=10)

    # ============== BGM Operations ==============

    def add_bgm(self):
        self._show_bgm_dialog(None)

    def edit_bgm(self, event=None):
        selection = self.bgm_tree.selection()
        if selection:
            item = self.bgm_tree.item(selection[0])
            name = item['values'][0]
            self._show_bgm_dialog(name)

    def delete_bgm(self):
        selection = self.bgm_tree.selection()
        if selection:
            item = self.bgm_tree.item(selection[0])
            name = item['values'][0]
            if messagebox.askyesno("Confirm", f"Delete BGM '{name}'?"):
                del self.bgm_mappings[name]
                self.refresh_bgm_tree()
                self.mark_modified()

    def _show_bgm_dialog(self, existing_name):
        dialog = tk.Toplevel(self.root)
        dialog.title("Edit BGM" if existing_name else "Add BGM")
        dialog.geometry("400x150")
        dialog.transient(self.root)
        dialog.grab_set()

        ttk.Label(dialog, text="Music Name:").pack(pady=(10, 5))
        name_entry = ttk.Entry(dialog, width=30)
        name_entry.pack(pady=5)

        path_frame = ttk.Frame(dialog)
        path_frame.pack(pady=5, fill=tk.X, padx=10)
        ttk.Label(path_frame, text="File Path:").pack(side=tk.LEFT)
        path_entry = ttk.Entry(path_frame, width=30)
        path_entry.pack(side=tk.LEFT, padx=5, expand=True, fill=tk.X)

        def browse():
            filepath = filedialog.askopenfilename(
                title="Select BGM File",
                initialdir=os.path.join(self.game_root, "bgm") if self.game_root else None,
                filetypes=[("Audio files", "*.ogg *.mp3 *.wav"), ("All files", "*.*")]
            )
            if filepath:
                if self.game_root and filepath.startswith(self.game_root):
                    filepath = filepath[len(self.game_root):].lstrip("/\\")
                path_entry.delete(0, tk.END)
                path_entry.insert(0, self.normalize_path(filepath))

        ttk.Button(path_frame, text="Browse", command=browse).pack(side=tk.LEFT, padx=5)

        if existing_name:
            name_entry.insert(0, existing_name)
            path_entry.insert(0, self.bgm_mappings[existing_name])

        def do_save():
            name = name_entry.get().strip()
            path = path_entry.get().strip()
            if not name or not path:
                return

            if existing_name and existing_name != name:
                del self.bgm_mappings[existing_name]
            self.bgm_mappings[name] = path

            self.refresh_bgm_tree()
            self.mark_modified()
            dialog.destroy()

        ttk.Button(dialog, text="Save", command=do_save).pack(pady=10)

    # ============== Shader Operations ==============

    def add_shader(self):
        self._show_shader_dialog(None)

    def edit_shader(self, event=None):
        selection = self.shaders_tree.selection()
        if selection:
            item = self.shaders_tree.item(selection[0])
            name = item['values'][0]
            self._show_shader_dialog(name)

    def delete_shader(self):
        selection = self.shaders_tree.selection()
        if selection:
            item = self.shaders_tree.item(selection[0])
            name = item['values'][0]
            if messagebox.askyesno("Confirm", f"Delete shader '{name}'?"):
                self.shaders = [s for s in self.shaders if s['name'] != name]
                self.refresh_shaders_tree()
                self.mark_modified()

    def move_shader_up(self):
        selection = self.shaders_tree.selection()
        if selection:
            item = self.shaders_tree.item(selection[0])
            name = item['values'][0]
            for i, s in enumerate(self.shaders):
                if s['name'] == name and i > 0:
                    self.shaders[i], self.shaders[i - 1] = self.shaders[i - 1], self.shaders[i]
                    self.refresh_shaders_tree()
                    self.mark_modified()
                    # Reselect the moved item
                    for child in self.shaders_tree.get_children():
                        if self.shaders_tree.item(child)['values'][0] == name:
                            self.shaders_tree.selection_set(child)
                            break
                    break

    def move_shader_down(self):
        selection = self.shaders_tree.selection()
        if selection:
            item = self.shaders_tree.item(selection[0])
            name = item['values'][0]
            for i, s in enumerate(self.shaders):
                if s['name'] == name and i < len(self.shaders) - 1:
                    self.shaders[i], self.shaders[i + 1] = self.shaders[i + 1], self.shaders[i]
                    self.refresh_shaders_tree()
                    self.mark_modified()
                    # Reselect the moved item
                    for child in self.shaders_tree.get_children():
                        if self.shaders_tree.item(child)['values'][0] == name:
                            self.shaders_tree.selection_set(child)
                            break
                    break

    def _show_shader_dialog(self, existing_name):
        dialog = tk.Toplevel(self.root)
        dialog.title("Edit Shader" if existing_name else "Add Shader")
        dialog.geometry("400x200")
        dialog.transient(self.root)
        dialog.grab_set()

        existing_shader = None
        if existing_name:
            for s in self.shaders:
                if s['name'] == existing_name:
                    existing_shader = s
                    break

        ttk.Label(dialog, text="Shader Name:").pack(pady=(10, 5))
        name_entry = ttk.Entry(dialog, width=30)
        name_entry.pack(pady=5)

        vert_frame = ttk.Frame(dialog)
        vert_frame.pack(pady=5, fill=tk.X, padx=10)
        ttk.Label(vert_frame, text="Vertex File:").pack(side=tk.LEFT)
        vert_entry = ttk.Entry(vert_frame, width=25)
        vert_entry.pack(side=tk.LEFT, padx=5, expand=True, fill=tk.X)

        frag_frame = ttk.Frame(dialog)
        frag_frame.pack(pady=5, fill=tk.X, padx=10)
        ttk.Label(frag_frame, text="Fragment File:").pack(side=tk.LEFT)
        frag_entry = ttk.Entry(frag_frame, width=25)
        frag_entry.pack(side=tk.LEFT, padx=5, expand=True, fill=tk.X)

        if existing_shader:
            name_entry.insert(0, existing_shader['name'])
            vert_entry.insert(0, existing_shader['vertex'])
            frag_entry.insert(0, existing_shader['fragment'])

        def do_save():
            name = name_entry.get().strip()
            vertex = vert_entry.get().strip()
            fragment = frag_entry.get().strip()
            if not name or not vertex or not fragment:
                return

            new_shader = {'name': name, 'vertex': vertex, 'fragment': fragment}

            if existing_name:
                for i, s in enumerate(self.shaders):
                    if s['name'] == existing_name:
                        self.shaders[i] = new_shader
                        break
            else:
                self.shaders.append(new_shader)

            self.refresh_shaders_tree()
            self.mark_modified()
            dialog.destroy()

        ttk.Button(dialog, text="Save", command=do_save).pack(pady=10)

    # ============== Renderer Operations ==============

    def add_renderer_setting(self):
        self._show_renderer_dialog(None)

    def edit_renderer_setting(self, event=None):
        selection = self.renderer_tree.selection()
        if selection:
            item = self.renderer_tree.item(selection[0])
            key = item['values'][0]
            self._show_renderer_dialog(key)

    def delete_renderer_setting(self):
        selection = self.renderer_tree.selection()
        if selection:
            item = self.renderer_tree.item(selection[0])
            key = item['values'][0]
            if messagebox.askyesno("Confirm", f"Delete setting '{key}'?"):
                del self.renderer_settings[key]
                self.refresh_renderer_tree()
                self.mark_modified()

    def _show_renderer_dialog(self, existing_key):
        dialog = tk.Toplevel(self.root)
        dialog.title("Edit Setting" if existing_key else "Add Setting")
        dialog.geometry("300x150")
        dialog.transient(self.root)
        dialog.grab_set()

        ttk.Label(dialog, text="Setting Name:").pack(pady=(10, 5))
        key_entry = ttk.Entry(dialog, width=25)
        key_entry.pack(pady=5)

        ttk.Label(dialog, text="Value:").pack(pady=5)
        value_entry = ttk.Entry(dialog, width=25)
        value_entry.pack(pady=5)

        if existing_key:
            key_entry.insert(0, existing_key)
            value_entry.insert(0, self.renderer_settings[existing_key])

        def do_save():
            key = key_entry.get().strip()
            value = value_entry.get().strip()
            if not key:
                return

            if existing_key and existing_key != key:
                del self.renderer_settings[existing_key]
            self.renderer_settings[key] = value

            self.refresh_renderer_tree()
            self.mark_modified()
            dialog.destroy()

        ttk.Button(dialog, text="Save", command=do_save).pack(pady=10)

    # ============== Settings Names Operations ==============

    def add_setting_name(self):
        self._show_setting_name_dialog(None)

    def edit_setting_name(self, event=None):
        selection = self.settings_names_tree.selection()
        if selection:
            item = self.settings_names_tree.item(selection[0])
            setting = item['values'][0]
            self._show_setting_name_dialog(setting)

    def delete_setting_name(self):
        selection = self.settings_names_tree.selection()
        if selection:
            item = self.settings_names_tree.item(selection[0])
            setting = item['values'][0]
            if messagebox.askyesno("Confirm", f"Delete setting '{setting}'?"):
                del self.settings_names[setting]
                self.refresh_settings_names_tree()
                self.mark_modified()

    def _show_setting_name_dialog(self, existing_setting):
        dialog = tk.Toplevel(self.root)
        dialog.title("Edit Setting Name" if existing_setting else "Add Setting Name")
        dialog.geometry("500x180")
        dialog.transient(self.root)
        dialog.grab_set()

        ttk.Label(dialog, text="Setting Name:").pack(pady=(10, 5))
        setting_entry = ttk.Entry(dialog, width=40)
        setting_entry.pack(pady=5)

        ttk.Label(dialog, text="Options (comma-separated):").pack(pady=5)
        options_entry = ttk.Entry(dialog, width=50)
        options_entry.pack(pady=5)

        if existing_setting:
            setting_entry.insert(0, existing_setting)
            options_entry.insert(0, self.settings_names[existing_setting])

        def do_save():
            setting = setting_entry.get().strip()
            options = options_entry.get().strip()
            if not setting:
                return

            if existing_setting and existing_setting != setting:
                del self.settings_names[existing_setting]
            self.settings_names[setting] = options

            self.refresh_settings_names_tree()
            self.mark_modified()
            dialog.destroy()

        ttk.Button(dialog, text="Save", command=do_save).pack(pady=10)

    # ============== Common Methods ==============

    def mark_modified(self):
        self.modified = True
        self.update_title()

    def update_title(self):
        title = "Config Editor"
        if self.game_root:
            project_name = os.path.basename(self.game_root)
            modified = "*" if self.modified else ""
            title += f" - {project_name}{modified}"
        self.root.title(title)

    def on_close(self):
        if self.modified:
            result = messagebox.askyesnocancel("Unsaved Changes", "Save changes before closing?")
            if result is None:
                return
            if result:
                self.save_all()
        self.root.destroy()


def main():
    root = tk.Tk()
    app = ConfigEditor(root)
    root.protocol("WM_DELETE_WINDOW", app.on_close)
    root.mainloop()


if __name__ == "__main__":
    main()
