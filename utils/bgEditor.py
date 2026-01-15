"""
Background Editor - GUI for editing bg.dat files
Manages parallax background layers for game scenes.

Format:
* name width height param x_offset y_offset
x y filepath z parallax [scaleX scaleY R G B A]
"""

import tkinter as tk
from tkinter import ttk, filedialog, messagebox
import os

# Try to import PIL for image preview
try:
    from PIL import Image, ImageTk
    HAS_PIL = True
except ImportError:
    HAS_PIL = False


class BackgroundEditor:
    def __init__(self, root):
        self.root = root
        self.root.title("Background Editor")
        self.root.geometry("1200x750")

        # Project data
        self.game_root = None
        self.modified = False

        # Background data
        self.backgrounds = {}  # name -> {params: {...}, layers: [...]}

        # Preview data
        self.preview_image = None
        self.preview_photo = None

        self.create_menu()
        self.create_toolbar()
        self.create_main_ui()
        self.create_status_bar()

    def create_menu(self):
        menubar = tk.Menu(self.root)
        self.root.config(menu=menubar)

        file_menu = tk.Menu(menubar, tearoff=0)
        menubar.add_cascade(label="File", menu=file_menu)
        file_menu.add_command(label="Open Project...", command=self.open_project, accelerator="Ctrl+O")
        file_menu.add_separator()
        file_menu.add_command(label="Save", command=self.save, accelerator="Ctrl+S")
        file_menu.add_separator()
        file_menu.add_command(label="Exit", command=self.on_close)

        self.root.bind("<Control-o>", lambda e: self.open_project())
        self.root.bind("<Control-s>", lambda e: self.save())

    def create_toolbar(self):
        toolbar = ttk.Frame(self.root)
        toolbar.pack(fill=tk.X, padx=5, pady=5)

        ttk.Button(toolbar, text="Open Project", command=self.open_project).pack(side=tk.LEFT, padx=2)
        ttk.Button(toolbar, text="Save", command=self.save).pack(side=tk.LEFT, padx=2)

        ttk.Separator(toolbar, orient=tk.VERTICAL).pack(side=tk.LEFT, fill=tk.Y, padx=10)

        self.project_path_var = tk.StringVar(value="No project open")
        ttk.Label(toolbar, textvariable=self.project_path_var, foreground="gray").pack(side=tk.LEFT, padx=5)

    def create_main_ui(self):
        main_paned = ttk.PanedWindow(self.root, orient=tk.HORIZONTAL)
        main_paned.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        # Left: Background list
        left_frame = ttk.LabelFrame(main_paned, text="Backgrounds")
        main_paned.add(left_frame, weight=1)

        bg_btn_frame = ttk.Frame(left_frame)
        bg_btn_frame.pack(fill=tk.X, padx=5, pady=5)
        ttk.Button(bg_btn_frame, text="Add", command=self.add_background).pack(side=tk.LEFT, padx=2)
        ttk.Button(bg_btn_frame, text="Delete", command=self.delete_background).pack(side=tk.LEFT, padx=2)
        ttk.Button(bg_btn_frame, text="Edit Params", command=self.edit_background_params).pack(side=tk.LEFT, padx=2)

        bg_list_frame = ttk.Frame(left_frame)
        bg_list_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        self.bg_listbox = tk.Listbox(bg_list_frame, font=("Consolas", 11), exportselection=False)
        bg_scroll = ttk.Scrollbar(bg_list_frame, orient=tk.VERTICAL, command=self.bg_listbox.yview)
        self.bg_listbox.configure(yscrollcommand=bg_scroll.set)

        self.bg_listbox.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        bg_scroll.pack(side=tk.RIGHT, fill=tk.Y)

        self.bg_listbox.bind("<<ListboxSelect>>", self.on_background_selected)
        self.bg_listbox.bind("<Double-1>", lambda e: self.edit_background_params())

        # Right: Notebook with Layers and Preview tabs
        right_frame = ttk.Frame(main_paned)
        main_paned.add(right_frame, weight=4)

        # Background params display (always visible)
        params_frame = ttk.LabelFrame(right_frame, text="Background Parameters")
        params_frame.pack(fill=tk.X, padx=5, pady=5)

        params_inner = ttk.Frame(params_frame)
        params_inner.pack(fill=tk.X, padx=5, pady=5)

        self.params_labels = {}
        for i, label in enumerate(["Width:", "Height:", "Param:", "X Offset:", "Y Offset:"]):
            ttk.Label(params_inner, text=label).grid(row=0, column=i*2, padx=5, sticky=tk.E)
            var = tk.StringVar(value="-")
            self.params_labels[label] = var
            ttk.Label(params_inner, textvariable=var, font=("Consolas", 10)).grid(row=0, column=i*2+1, padx=5, sticky=tk.W)

        # Notebook for tabs
        self.notebook = ttk.Notebook(right_frame)
        self.notebook.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        # Tab 1: Layers
        self.layers_tab = ttk.Frame(self.notebook)
        self.notebook.add(self.layers_tab, text="Layers")
        self.create_layers_tab()

        # Tab 2: Preview
        self.preview_tab = ttk.Frame(self.notebook)
        self.notebook.add(self.preview_tab, text="Preview")
        self.create_preview_tab()

        # Bind tab change to update preview
        self.notebook.bind("<<NotebookTabChanged>>", self.on_tab_changed)

    def create_layers_tab(self):
        layer_btn_frame = ttk.Frame(self.layers_tab)
        layer_btn_frame.pack(fill=tk.X, padx=5, pady=5)
        ttk.Button(layer_btn_frame, text="Add Layer", command=self.add_layer).pack(side=tk.LEFT, padx=2)
        ttk.Button(layer_btn_frame, text="Delete", command=self.delete_layer).pack(side=tk.LEFT, padx=2)
        ttk.Button(layer_btn_frame, text="Move Up", command=self.move_layer_up).pack(side=tk.LEFT, padx=2)
        ttk.Button(layer_btn_frame, text="Move Down", command=self.move_layer_down).pack(side=tk.LEFT, padx=2)

        layer_list_frame = ttk.Frame(self.layers_tab)
        layer_list_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        columns = ("x", "y", "filepath", "z", "parallax", "scaleX", "scaleY", "color")
        self.layer_tree = ttk.Treeview(layer_list_frame, columns=columns, show="headings")
        self.layer_tree.heading("x", text="X")
        self.layer_tree.heading("y", text="Y")
        self.layer_tree.heading("filepath", text="Image Path")
        self.layer_tree.heading("z", text="Z")
        self.layer_tree.heading("parallax", text="Parallax")
        self.layer_tree.heading("scaleX", text="Scale X")
        self.layer_tree.heading("scaleY", text="Scale Y")
        self.layer_tree.heading("color", text="RGBA")

        self.layer_tree.column("x", width=50)
        self.layer_tree.column("y", width=50)
        self.layer_tree.column("filepath", width=300)
        self.layer_tree.column("z", width=50)
        self.layer_tree.column("parallax", width=70)
        self.layer_tree.column("scaleX", width=70)
        self.layer_tree.column("scaleY", width=70)
        self.layer_tree.column("color", width=100)

        layer_scroll = ttk.Scrollbar(layer_list_frame, orient=tk.VERTICAL, command=self.layer_tree.yview)
        self.layer_tree.configure(yscrollcommand=layer_scroll.set)

        self.layer_tree.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        layer_scroll.pack(side=tk.RIGHT, fill=tk.Y)

        self.layer_tree.bind("<Double-1>", self.edit_layer)

    def create_preview_tab(self):
        # Preview controls
        ctrl_frame = ttk.Frame(self.preview_tab)
        ctrl_frame.pack(fill=tk.X, padx=5, pady=5)

        ttk.Button(ctrl_frame, text="Refresh Preview", command=self.refresh_preview).pack(side=tk.LEFT, padx=2)

        self.preview_scale_var = tk.StringVar(value="50%")
        ttk.Label(ctrl_frame, text="Scale:").pack(side=tk.LEFT, padx=(20, 5))
        scale_combo = ttk.Combobox(ctrl_frame, textvariable=self.preview_scale_var,
                                   values=["25%", "50%", "75%", "100%"], width=8, state="readonly")
        scale_combo.pack(side=tk.LEFT, padx=2)
        scale_combo.bind("<<ComboboxSelected>>", lambda e: self.refresh_preview())

        self.preview_info_var = tk.StringVar(value="")
        ttk.Label(ctrl_frame, textvariable=self.preview_info_var, foreground="gray").pack(side=tk.LEFT, padx=20)

        if not HAS_PIL:
            ttk.Label(ctrl_frame, text="(Install Pillow for preview: pip install Pillow)",
                     foreground="red").pack(side=tk.RIGHT, padx=5)

        # Preview canvas with scrollbars
        canvas_frame = ttk.Frame(self.preview_tab)
        canvas_frame.pack(fill=tk.BOTH, expand=True, padx=5, pady=5)

        self.preview_canvas = tk.Canvas(canvas_frame, bg="#333333", highlightthickness=0)

        h_scroll = ttk.Scrollbar(canvas_frame, orient=tk.HORIZONTAL, command=self.preview_canvas.xview)
        v_scroll = ttk.Scrollbar(canvas_frame, orient=tk.VERTICAL, command=self.preview_canvas.yview)
        self.preview_canvas.configure(xscrollcommand=h_scroll.set, yscrollcommand=v_scroll.set)

        h_scroll.pack(side=tk.BOTTOM, fill=tk.X)
        v_scroll.pack(side=tk.RIGHT, fill=tk.Y)
        self.preview_canvas.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

    def create_status_bar(self):
        self.status_var = tk.StringVar(value="Ready - Open a project to begin")
        status_bar = ttk.Label(self.root, textvariable=self.status_var, relief=tk.SUNKEN, anchor=tk.W)
        status_bar.pack(side=tk.BOTTOM, fill=tk.X)

    # ============== Preview Operations ==============

    def on_tab_changed(self, event=None):
        """Called when notebook tab changes."""
        current_tab = self.notebook.index(self.notebook.select())
        if current_tab == 1:  # Preview tab
            self.refresh_preview()

    def refresh_preview(self):
        """Refresh the preview canvas with the composed background."""
        if not HAS_PIL:
            self.preview_info_var.set("Pillow not installed")
            return

        selection = self.bg_listbox.curselection()
        if not selection:
            self.preview_canvas.delete("all")
            self.preview_info_var.set("No background selected")
            return

        bg_name = self.bg_listbox.get(selection[0])
        if bg_name not in self.backgrounds:
            return

        data = self.backgrounds[bg_name]
        params = data['params']
        layers = data['layers']

        if not layers:
            self.preview_canvas.delete("all")
            self.preview_info_var.set("No layers in background")
            return

        # Get scale factor
        scale_str = self.preview_scale_var.get()
        scale = int(scale_str.rstrip('%')) / 100.0

        # Compose the background
        try:
            composed = self.compose_background(bg_name)
            if composed is None:
                self.preview_info_var.set("Failed to compose background")
                return

            # Scale the image
            if scale != 1.0:
                new_width = int(composed.width * scale)
                new_height = int(composed.height * scale)
                composed = composed.resize((new_width, new_height), Image.Resampling.LANCZOS)

            # Convert to PhotoImage and display
            self.preview_photo = ImageTk.PhotoImage(composed)

            self.preview_canvas.delete("all")
            self.preview_canvas.create_image(0, 0, anchor=tk.NW, image=self.preview_photo)
            self.preview_canvas.configure(scrollregion=(0, 0, composed.width, composed.height))

            self.preview_info_var.set(f"Size: {composed.width}x{composed.height} | Layers: {len(layers)}")

        except Exception as e:
            self.preview_canvas.delete("all")
            self.preview_info_var.set(f"Error: {str(e)}")

    def compose_background(self, bg_name):
        """Compose all layers into a single image."""
        if not HAS_PIL or not self.game_root:
            return None

        data = self.backgrounds[bg_name]
        params = data['params']
        layers = data['layers']

        if not layers:
            return None

        # Determine canvas size - use background params or calculate from layers
        canvas_width = params.get('width', 640)
        canvas_height = params.get('height', 360)

        # Find bounds from all layers to ensure we capture everything
        min_x, min_y = 0, 0
        max_x, max_y = canvas_width, canvas_height

        loaded_layers = []

        for layer in layers:
            filepath = layer['filepath']
            full_path = os.path.join(self.game_root, filepath)

            if not os.path.exists(full_path):
                continue

            try:
                img = Image.open(full_path).convert("RGBA")

                # Apply scale if specified
                if layer['scaleX'] is not None and layer['scaleY'] is not None:
                    try:
                        scale_x = float(layer['scaleX'])
                        scale_y = float(layer['scaleY'])
                        new_width = int(img.width * scale_x)
                        new_height = int(img.height * scale_y)
                        if new_width > 0 and new_height > 0:
                            img = img.resize((new_width, new_height), Image.Resampling.LANCZOS)
                    except ValueError:
                        pass

                # Apply color tint if specified
                if layer['r'] is not None:
                    img = self.apply_color_tint(img, layer['r'], layer['g'], layer['b'], layer['a'])

                # Get z-order for sorting
                try:
                    z = int(layer['z'])
                except ValueError:
                    z = 0

                x = layer['x']
                y = layer['y']

                loaded_layers.append({
                    'image': img,
                    'x': x,
                    'y': y,
                    'z': z
                })

                # Update bounds
                min_x = min(min_x, x)
                min_y = min(min_y, y)
                max_x = max(max_x, x + img.width)
                max_y = max(max_y, y + img.height)

            except Exception as e:
                print(f"Failed to load layer {filepath}: {e}")
                continue

        if not loaded_layers:
            return None

        # Sort by z-order (lower z = further back, drawn first)
        loaded_layers.sort(key=lambda l: l['z'], reverse=True)

        # Create canvas with calculated size
        final_width = max(canvas_width, max_x - min_x)
        final_height = max(canvas_height, max_y - min_y)

        # Clamp to reasonable size
        final_width = min(final_width, 4096)
        final_height = min(final_height, 4096)

        canvas = Image.new("RGBA", (final_width, final_height), (0, 0, 0, 255))

        # Offset to handle negative positions
        offset_x = -min_x if min_x < 0 else 0
        offset_y = -min_y if min_y < 0 else 0

        # Composite layers
        for layer_data in loaded_layers:
            img = layer_data['image']
            x = layer_data['x'] + offset_x
            y = layer_data['y'] + offset_y

            # Ensure position is valid
            if x < 0:
                img = img.crop((-x, 0, img.width, img.height))
                x = 0
            if y < 0:
                img = img.crop((0, -y, img.width, img.height))
                y = 0

            # Paste with alpha compositing
            if x < canvas.width and y < canvas.height:
                # Create a temporary canvas to handle overflow
                temp = Image.new("RGBA", canvas.size, (0, 0, 0, 0))
                temp.paste(img, (x, y))
                canvas = Image.alpha_composite(canvas, temp)

        return canvas

    def apply_color_tint(self, img, r, g, b, a):
        """Apply RGBA color tint to an image."""
        if img.mode != "RGBA":
            img = img.convert("RGBA")

        # Create a colored overlay
        overlay = Image.new("RGBA", img.size, (r, g, b, a))

        # Blend based on alpha
        result = Image.new("RGBA", img.size)
        for x in range(img.width):
            for y in range(img.height):
                px = img.getpixel((x, y))
                if px[3] > 0:  # If pixel is not fully transparent
                    # Simple color multiplication
                    new_r = int(px[0] * r / 255)
                    new_g = int(px[1] * g / 255)
                    new_b = int(px[2] * b / 255)
                    new_a = int(px[3] * a / 255)
                    result.putpixel((x, y), (new_r, new_g, new_b, new_a))
                else:
                    result.putpixel((x, y), px)

        return result

    # ============== Project Operations ==============

    def normalize_path(self, path):
        return path.replace("\\", "/")

    def open_project(self):
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
            self.load_backgrounds()
            self.status_var.set(f"Opened project: {self.normalize_path(folder)}")

    def load_backgrounds(self):
        """Load backgrounds from bg.dat."""
        self.backgrounds.clear()
        bg_path = os.path.join(self.game_root, "data", "config", "bg.dat")

        if not os.path.exists(bg_path):
            self.refresh_bg_listbox()
            return

        try:
            with open(bg_path, 'r') as f:
                current_bg = None
                for line in f:
                    line = line.rstrip('\n')
                    if not line:
                        continue

                    if line.startswith('* '):
                        # Parse background header
                        parts = line[2:].split()
                        name = parts[0]
                        params = {
                            'width': int(parts[1]) if len(parts) > 1 else 0,
                            'height': int(parts[2]) if len(parts) > 2 else 0,
                            'param': int(parts[3]) if len(parts) > 3 else 1,
                            'x_offset': int(parts[4]) if len(parts) > 4 else 0,
                            'y_offset': int(parts[5]) if len(parts) > 5 else 0,
                        }
                        self.backgrounds[name] = {'params': params, 'layers': []}
                        current_bg = name
                    elif current_bg:
                        # Parse layer
                        layer = self._parse_layer(line)
                        if layer:
                            self.backgrounds[current_bg]['layers'].append(layer)

        except Exception as e:
            messagebox.showerror("Error", f"Failed to load bg.dat:\n{e}")

        self.refresh_bg_listbox()
        self.modified = False
        self.update_title()

    def _parse_layer(self, line):
        """Parse a layer line."""
        parts = line.split()
        if len(parts) < 4:
            return None

        layer = {
            'x': int(parts[0]),
            'y': int(parts[1]),
            'filepath': parts[2],
            'z': parts[3].rstrip(','),  # Remove trailing comma if present
            'parallax': parts[4].rstrip('f') if len(parts) > 4 else '0.0',
            'scaleX': None,
            'scaleY': None,
            'r': None,
            'g': None,
            'b': None,
            'a': None
        }

        # Check for extended parameters (scale and color)
        if len(parts) > 5:
            layer['scaleX'] = parts[5].rstrip('f')
        if len(parts) > 6:
            layer['scaleY'] = parts[6].rstrip('f')
        if len(parts) > 7:
            layer['r'] = int(parts[7])
        if len(parts) > 8:
            layer['g'] = int(parts[8])
        if len(parts) > 9:
            layer['b'] = int(parts[9])
        if len(parts) > 10:
            layer['a'] = int(parts[10])

        return layer

    def save(self):
        """Save backgrounds to bg.dat."""
        if not self.game_root:
            messagebox.showwarning("Warning", "No project open")
            return

        bg_path = os.path.join(self.game_root, "data", "config", "bg.dat")
        try:
            with open(bg_path, 'w') as f:
                for name, data in self.backgrounds.items():
                    params = data['params']
                    f.write(f"* {name} {params['width']} {params['height']} {params['param']} {params['x_offset']} {params['y_offset']}\n")

                    for layer in data['layers']:
                        line = f"{layer['x']} {layer['y']} {layer['filepath']} {layer['z']} {layer['parallax']}f"

                        # Add optional scale/color parameters
                        if layer['scaleX'] is not None:
                            line += f" {layer['scaleX']}f {layer['scaleY']}f"
                            if layer['r'] is not None:
                                line += f" {layer['r']} {layer['g']} {layer['b']} {layer['a']}"

                        f.write(line + "\n")

            self.modified = False
            self.update_title()
            self.status_var.set("Saved bg.dat")

        except Exception as e:
            messagebox.showerror("Error", f"Failed to save bg.dat:\n{e}")

    # ============== Refresh UI ==============

    def refresh_bg_listbox(self):
        self.bg_listbox.delete(0, tk.END)
        for name in self.backgrounds.keys():
            self.bg_listbox.insert(tk.END, name)

    def refresh_layers_tree(self):
        self.layer_tree.delete(*self.layer_tree.get_children())

        selection = self.bg_listbox.curselection()
        if not selection:
            for label in self.params_labels.values():
                label.set("-")
            return

        bg_name = self.bg_listbox.get(selection[0])
        if bg_name not in self.backgrounds:
            return

        data = self.backgrounds[bg_name]

        # Update params display
        params = data['params']
        self.params_labels["Width:"].set(str(params['width']))
        self.params_labels["Height:"].set(str(params['height']))
        self.params_labels["Param:"].set(str(params['param']))
        self.params_labels["X Offset:"].set(str(params['x_offset']))
        self.params_labels["Y Offset:"].set(str(params['y_offset']))

        # Update layers tree
        for layer in data['layers']:
            color = ""
            if layer['r'] is not None:
                color = f"{layer['r']},{layer['g']},{layer['b']},{layer['a']}"

            self.layer_tree.insert("", tk.END, values=(
                layer['x'],
                layer['y'],
                layer['filepath'],
                layer['z'],
                layer['parallax'],
                layer['scaleX'] or "",
                layer['scaleY'] or "",
                color
            ))

    def on_background_selected(self, event=None):
        self.refresh_layers_tree()
        # Auto-refresh preview if on preview tab
        current_tab = self.notebook.index(self.notebook.select())
        if current_tab == 1:
            self.refresh_preview()

    # ============== Background Operations ==============

    def add_background(self):
        dialog = tk.Toplevel(self.root)
        dialog.title("Add Background")
        dialog.geometry("350x300")
        dialog.transient(self.root)
        dialog.grab_set()

        ttk.Label(dialog, text="Background Name:").pack(pady=(10, 5))
        name_entry = ttk.Entry(dialog, width=25)
        name_entry.pack(pady=5)
        name_entry.focus_set()

        params_frame = ttk.LabelFrame(dialog, text="Parameters")
        params_frame.pack(fill=tk.X, padx=10, pady=10)

        entries = {}
        defaults = {'width': 640, 'height': 360, 'param': 1, 'x_offset': 640, 'y_offset': 360}
        for i, (label, default) in enumerate(defaults.items()):
            ttk.Label(params_frame, text=f"{label.replace('_', ' ').title()}:").grid(row=i, column=0, padx=5, pady=2, sticky=tk.E)
            entry = ttk.Entry(params_frame, width=15)
            entry.insert(0, str(default))
            entry.grid(row=i, column=1, padx=5, pady=2, sticky=tk.W)
            entries[label] = entry

        def do_add():
            name = name_entry.get().strip()
            if not name:
                return
            if name in self.backgrounds:
                messagebox.showwarning("Warning", f"Background '{name}' already exists")
                return

            params = {}
            for key, entry in entries.items():
                try:
                    params[key] = int(entry.get().strip())
                except ValueError:
                    params[key] = defaults[key]

            self.backgrounds[name] = {'params': params, 'layers': []}
            self.refresh_bg_listbox()
            self.mark_modified()
            dialog.destroy()

        name_entry.bind("<Return>", lambda e: do_add())
        ttk.Button(dialog, text="Add", command=do_add).pack(pady=10)

    def delete_background(self):
        selection = self.bg_listbox.curselection()
        if selection:
            name = self.bg_listbox.get(selection[0])
            if messagebox.askyesno("Confirm", f"Delete background '{name}'?"):
                del self.backgrounds[name]
                self.refresh_bg_listbox()
                self.refresh_layers_tree()
                self.mark_modified()

    def edit_background_params(self):
        selection = self.bg_listbox.curselection()
        if not selection:
            return

        bg_name = self.bg_listbox.get(selection[0])
        data = self.backgrounds[bg_name]

        dialog = tk.Toplevel(self.root)
        dialog.title(f"Edit Parameters - {bg_name}")
        dialog.geometry("350x250")
        dialog.transient(self.root)
        dialog.grab_set()

        params_frame = ttk.LabelFrame(dialog, text="Parameters")
        params_frame.pack(fill=tk.X, padx=10, pady=10)

        entries = {}
        params = data['params']
        for i, (key, value) in enumerate(params.items()):
            label = key.replace('_', ' ').title()
            ttk.Label(params_frame, text=f"{label}:").grid(row=i, column=0, padx=5, pady=2, sticky=tk.E)
            entry = ttk.Entry(params_frame, width=15)
            entry.insert(0, str(value))
            entry.grid(row=i, column=1, padx=5, pady=2, sticky=tk.W)
            entries[key] = entry

        def do_save():
            for key, entry in entries.items():
                try:
                    params[key] = int(entry.get().strip())
                except ValueError:
                    pass

            self.refresh_layers_tree()
            self.mark_modified()
            dialog.destroy()

        ttk.Button(dialog, text="Save", command=do_save).pack(pady=10)

    # ============== Layer Operations ==============

    def add_layer(self):
        selection = self.bg_listbox.curselection()
        if not selection:
            messagebox.showwarning("Warning", "Select a background first")
            return

        bg_name = self.bg_listbox.get(selection[0])
        self._show_layer_dialog(bg_name, None)

    def edit_layer(self, event=None):
        bg_selection = self.bg_listbox.curselection()
        layer_selection = self.layer_tree.selection()

        if bg_selection and layer_selection:
            bg_name = self.bg_listbox.get(bg_selection[0])
            item = self.layer_tree.item(layer_selection[0])
            filepath = item['values'][2]  # filepath is unique identifier

            # Find layer index
            for i, layer in enumerate(self.backgrounds[bg_name]['layers']):
                if layer['filepath'] == filepath:
                    self._show_layer_dialog(bg_name, i)
                    break

    def delete_layer(self):
        bg_selection = self.bg_listbox.curselection()
        layer_selection = self.layer_tree.selection()

        if bg_selection and layer_selection:
            bg_name = self.bg_listbox.get(bg_selection[0])
            item = self.layer_tree.item(layer_selection[0])
            filepath = item['values'][2]

            if messagebox.askyesno("Confirm", f"Delete layer '{filepath}'?"):
                self.backgrounds[bg_name]['layers'] = [
                    l for l in self.backgrounds[bg_name]['layers'] if l['filepath'] != filepath
                ]
                self.refresh_layers_tree()
                self.mark_modified()

    def move_layer_up(self):
        bg_selection = self.bg_listbox.curselection()
        layer_selection = self.layer_tree.selection()

        if bg_selection and layer_selection:
            bg_name = self.bg_listbox.get(bg_selection[0])
            item = self.layer_tree.item(layer_selection[0])
            filepath = item['values'][2]

            layers = self.backgrounds[bg_name]['layers']
            for i, layer in enumerate(layers):
                if layer['filepath'] == filepath and i > 0:
                    layers[i], layers[i - 1] = layers[i - 1], layers[i]
                    self.refresh_layers_tree()
                    self.mark_modified()
                    # Reselect
                    for child in self.layer_tree.get_children():
                        if self.layer_tree.item(child)['values'][2] == filepath:
                            self.layer_tree.selection_set(child)
                            break
                    break

    def move_layer_down(self):
        bg_selection = self.bg_listbox.curselection()
        layer_selection = self.layer_tree.selection()

        if bg_selection and layer_selection:
            bg_name = self.bg_listbox.get(bg_selection[0])
            item = self.layer_tree.item(layer_selection[0])
            filepath = item['values'][2]

            layers = self.backgrounds[bg_name]['layers']
            for i, layer in enumerate(layers):
                if layer['filepath'] == filepath and i < len(layers) - 1:
                    layers[i], layers[i + 1] = layers[i + 1], layers[i]
                    self.refresh_layers_tree()
                    self.mark_modified()
                    # Reselect
                    for child in self.layer_tree.get_children():
                        if self.layer_tree.item(child)['values'][2] == filepath:
                            self.layer_tree.selection_set(child)
                            break
                    break

    def _show_layer_dialog(self, bg_name, layer_index):
        existing_layer = None
        if layer_index is not None:
            existing_layer = self.backgrounds[bg_name]['layers'][layer_index]

        dialog = tk.Toplevel(self.root)
        dialog.title("Edit Layer" if existing_layer else "Add Layer")
        dialog.geometry("500x400")
        dialog.transient(self.root)
        dialog.grab_set()

        # Position
        pos_frame = ttk.LabelFrame(dialog, text="Position")
        pos_frame.pack(fill=tk.X, padx=10, pady=5)

        ttk.Label(pos_frame, text="X:").grid(row=0, column=0, padx=5, pady=2, sticky=tk.E)
        x_entry = ttk.Entry(pos_frame, width=10)
        x_entry.grid(row=0, column=1, padx=5, pady=2, sticky=tk.W)

        ttk.Label(pos_frame, text="Y:").grid(row=0, column=2, padx=5, pady=2, sticky=tk.E)
        y_entry = ttk.Entry(pos_frame, width=10)
        y_entry.grid(row=0, column=3, padx=5, pady=2, sticky=tk.W)

        # Filepath
        path_frame = ttk.LabelFrame(dialog, text="Image")
        path_frame.pack(fill=tk.X, padx=10, pady=5)

        path_entry = ttk.Entry(path_frame, width=50)
        path_entry.pack(side=tk.LEFT, padx=5, pady=5, expand=True, fill=tk.X)

        def browse():
            filepath = filedialog.askopenfilename(
                title="Select Image",
                initialdir=os.path.join(self.game_root, "assets") if self.game_root else None,
                filetypes=[("Image files", "*.png *.jpg *.jpeg *.bmp"), ("All files", "*.*")]
            )
            if filepath:
                if self.game_root and filepath.startswith(self.game_root):
                    filepath = filepath[len(self.game_root):].lstrip("/\\")
                path_entry.delete(0, tk.END)
                path_entry.insert(0, self.normalize_path(filepath))

        ttk.Button(path_frame, text="Browse", command=browse).pack(side=tk.LEFT, padx=5, pady=5)

        # Z and Parallax
        render_frame = ttk.LabelFrame(dialog, text="Rendering")
        render_frame.pack(fill=tk.X, padx=10, pady=5)

        ttk.Label(render_frame, text="Z Order:").grid(row=0, column=0, padx=5, pady=2, sticky=tk.E)
        z_entry = ttk.Entry(render_frame, width=10)
        z_entry.grid(row=0, column=1, padx=5, pady=2, sticky=tk.W)

        ttk.Label(render_frame, text="Parallax:").grid(row=0, column=2, padx=5, pady=2, sticky=tk.E)
        parallax_entry = ttk.Entry(render_frame, width=10)
        parallax_entry.grid(row=0, column=3, padx=5, pady=2, sticky=tk.W)

        # Scale (optional)
        scale_frame = ttk.LabelFrame(dialog, text="Scale (Optional)")
        scale_frame.pack(fill=tk.X, padx=10, pady=5)

        ttk.Label(scale_frame, text="Scale X:").grid(row=0, column=0, padx=5, pady=2, sticky=tk.E)
        scaleX_entry = ttk.Entry(scale_frame, width=10)
        scaleX_entry.grid(row=0, column=1, padx=5, pady=2, sticky=tk.W)

        ttk.Label(scale_frame, text="Scale Y:").grid(row=0, column=2, padx=5, pady=2, sticky=tk.E)
        scaleY_entry = ttk.Entry(scale_frame, width=10)
        scaleY_entry.grid(row=0, column=3, padx=5, pady=2, sticky=tk.W)

        # Color (optional)
        color_frame = ttk.LabelFrame(dialog, text="Color Tint (Optional - RGBA 0-255)")
        color_frame.pack(fill=tk.X, padx=10, pady=5)

        color_entries = {}
        for i, c in enumerate(['R', 'G', 'B', 'A']):
            ttk.Label(color_frame, text=f"{c}:").grid(row=0, column=i*2, padx=5, pady=2, sticky=tk.E)
            entry = ttk.Entry(color_frame, width=6)
            entry.grid(row=0, column=i*2+1, padx=5, pady=2, sticky=tk.W)
            color_entries[c] = entry

        # Fill in existing values
        if existing_layer:
            x_entry.insert(0, str(existing_layer['x']))
            y_entry.insert(0, str(existing_layer['y']))
            path_entry.insert(0, existing_layer['filepath'])
            z_entry.insert(0, str(existing_layer['z']))
            parallax_entry.insert(0, str(existing_layer['parallax']))
            if existing_layer['scaleX'] is not None:
                scaleX_entry.insert(0, str(existing_layer['scaleX']))
                scaleY_entry.insert(0, str(existing_layer['scaleY']))
            if existing_layer['r'] is not None:
                color_entries['R'].insert(0, str(existing_layer['r']))
                color_entries['G'].insert(0, str(existing_layer['g']))
                color_entries['B'].insert(0, str(existing_layer['b']))
                color_entries['A'].insert(0, str(existing_layer['a']))
        else:
            x_entry.insert(0, "0")
            y_entry.insert(0, "0")
            z_entry.insert(0, "-10")
            parallax_entry.insert(0, "0.0")

        def do_save():
            filepath = path_entry.get().strip()
            if not filepath:
                messagebox.showerror("Error", "Image path is required")
                return

            try:
                x = int(x_entry.get().strip())
                y = int(y_entry.get().strip())
            except ValueError:
                x, y = 0, 0

            layer = {
                'x': x,
                'y': y,
                'filepath': filepath,
                'z': z_entry.get().strip() or "-10",
                'parallax': parallax_entry.get().strip() or "0.0",
                'scaleX': None,
                'scaleY': None,
                'r': None,
                'g': None,
                'b': None,
                'a': None
            }

            # Optional scale
            scaleX_val = scaleX_entry.get().strip()
            scaleY_val = scaleY_entry.get().strip()
            if scaleX_val and scaleY_val:
                layer['scaleX'] = scaleX_val
                layer['scaleY'] = scaleY_val

                # Optional color (only if scale is set)
                r_val = color_entries['R'].get().strip()
                g_val = color_entries['G'].get().strip()
                b_val = color_entries['B'].get().strip()
                a_val = color_entries['A'].get().strip()
                if r_val and g_val and b_val and a_val:
                    try:
                        layer['r'] = int(r_val)
                        layer['g'] = int(g_val)
                        layer['b'] = int(b_val)
                        layer['a'] = int(a_val)
                    except ValueError:
                        pass

            if existing_layer:
                self.backgrounds[bg_name]['layers'][layer_index] = layer
            else:
                self.backgrounds[bg_name]['layers'].append(layer)

            self.refresh_layers_tree()
            self.mark_modified()
            dialog.destroy()

        ttk.Button(dialog, text="Save", command=do_save).pack(pady=10)

    # ============== Common Methods ==============

    def mark_modified(self):
        self.modified = True
        self.update_title()

    def update_title(self):
        title = "Background Editor"
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
                self.save()
        self.root.destroy()


def main():
    root = tk.Tk()
    app = BackgroundEditor(root)
    root.protocol("WM_DELETE_WINDOW", app.on_close)
    root.mainloop()


if __name__ == "__main__":
    main()
