import tkinter as tk
from tkinter import filedialog, messagebox, ttk, simpledialog
import json
from PIL import Image
import pickle
import platform

class PixelArtApp:
    def __init__(self, root):
        self.root = root
        self.root.title("Editor de Trama - LoomETec")
        
        # Detectar sistema operativo para atajos
        self.is_mac = platform.system() == 'Darwin'
        self.mod_key = 'Command' if self.is_mac else 'Control'
        
        # Paleta de 4 colores para botones
        self.colores = ['#6b8c7e', '#f29c2e', '#f2551d', '#f16e64']
        
        self.columnas = 24
        self.filas = 24
        self.columnas_base = 24
        self.filas_base = 24
        self.size_pixel = 18
        self.color_seleccionado = "#000000"
        
        # Control de posición del pincel mejorado
        self.ultima_posicion_pincel = None
        self.pintando = False
        
        # Sistema de deshacer/rehacer
        self.historial = []
        self.historial_index = -1
        self.max_historial = 50
        
        # Pinceles predeterminados
        self.pinceles = {
            "Punto": [[1]],
            "Linea H": [[1, 1, 1]],
            "Linea V": [[1], [1], [1]],
            "Cruz": [[0, 1, 0], [1, 1, 1], [0, 1, 0]],
            "Cuadrado": [[1, 1], [1, 1]],
            "Diagonal": [[1, 0, 0], [0, 1, 0], [0, 0, 1]]
        }
        self.pincel_actual = "Punto"
        
        self.matriz_datos = [["#FFFFFF" for _ in range(self.columnas)] for _ in range(self.filas)]
        self.matriz_ids = [[None for _ in range(self.columnas)] for _ in range(self.filas)]

        self.setup_ui()
        self.actualizar_interfaz()
        self.guardar_estado()  
        self.setup_keyboard_shortcuts()

    def setup_keyboard_shortcuts(self):
        """Configura los atajos de teclado"""
        # Ctrl/Cmd + Z: Deshacer
        self.root.bind(f'<{self.mod_key}-z>', lambda e: self.deshacer())
        self.root.bind(f'<{self.mod_key}-Z>', lambda e: self.deshacer())
        
        # Ctrl/Cmd + Y o Ctrl/Cmd + Shift + Z: Rehacer
        self.root.bind(f'<{self.mod_key}-y>', lambda e: self.rehacer())
        self.root.bind(f'<{self.mod_key}-Y>', lambda e: self.rehacer())
        self.root.bind(f'<{self.mod_key}-Shift-z>', lambda e: self.rehacer())
        self.root.bind(f'<{self.mod_key}-Shift-Z>', lambda e: self.rehacer())
        
        # Ctrl/Cmd + L: Limpiar lienzo
        self.root.bind(f'<{self.mod_key}-l>', lambda e: self.limpiar_lienzo())
        self.root.bind(f'<{self.mod_key}-L>', lambda e: self.limpiar_lienzo())
        
        # Ctrl/Cmd + Plus: Agrandar
        self.root.bind(f'<{self.mod_key}-plus>', lambda e: self.ajustar_tamano(4))
        self.root.bind(f'<{self.mod_key}-equal>', lambda e: self.ajustar_tamano(4))  # + sin shift
        self.root.bind(f'<{self.mod_key}-KP_Add>', lambda e: self.ajustar_tamano(4))  # + del teclado numérico
        
        # Ctrl/Cmd + Minus: Achicar
        self.root.bind(f'<{self.mod_key}-minus>', lambda e: self.ajustar_tamano(-4))
        self.root.bind(f'<{self.mod_key}-KP_Subtract>', lambda e: self.ajustar_tamano(-4))  # - del teclado numérico
        
        # Ctrl/Cmd + S: Guardar PNG
        self.root.bind(f'<{self.mod_key}-s>', lambda e: self.guardar_png())
        self.root.bind(f'<{self.mod_key}-S>', lambda e: self.guardar_png())
        
        # Ctrl/Cmd + J: Guardar JSON
        self.root.bind(f'<{self.mod_key}-j>', lambda e: self.guardar_json())
        self.root.bind(f'<{self.mod_key}-J>', lambda e: self.guardar_json())

    def guardar_estado(self):
        """Guarda el estado actual en el historial"""
        # Eliminar estados futuros si estamos en medio del historial
        if self.historial_index < len(self.historial) - 1:
            self.historial = self.historial[:self.historial_index + 1]
        
        # Guardar copia profunda del estado
        estado = {
            'matriz': [fila[:] for fila in self.matriz_datos],
            'filas': self.filas,
            'columnas': self.columnas
        }
        self.historial.append(estado)
        
        # Limitar tamaño del historial
        if len(self.historial) > self.max_historial:
            self.historial.pop(0)
        else:
            self.historial_index += 1

    def deshacer(self):
        """Deshace la última acción"""
        if self.historial_index > 0:
            self.historial_index -= 1
            estado = self.historial[self.historial_index]
            self.matriz_datos = [fila[:] for fila in estado['matriz']]
            self.filas = estado['filas']
            self.columnas = estado['columnas']
            self.actualizar_interfaz()
            self.root.title(f"Editor de Trama - LoomETec [Deshacer: {self.historial_index}/{len(self.historial)-1}]")

    def rehacer(self):
        """Rehace la última acción deshecha"""
        if self.historial_index < len(self.historial) - 1:
            self.historial_index += 1
            estado = self.historial[self.historial_index]
            self.matriz_datos = [fila[:] for fila in estado['matriz']]
            self.filas = estado['filas']
            self.columnas = estado['columnas']
            self.actualizar_interfaz()
            self.root.title(f"Editor de Trama - LoomETec [Rehacer: {self.historial_index}/{len(self.historial)-1}]")

    def setup_ui(self):
        # Estilo ttk moderno y limpio
        style = ttk.Style()
        style.theme_use('clam')
        
        style.configure('Custom.TButton', padding=10, font=('Helvetica', 10, 'bold'))
        style.map('Custom.TButton',
                  background=[('active', '#dddddd')],
                  foreground=[('active', '#000000')])
        
        style.configure('Verde.TButton', background=self.colores[0], foreground='white')
        style.configure('Naranja.TButton', background=self.colores[1], foreground='white')
        style.configure('Rojo.TButton', background=self.colores[2], foreground='white')
        style.configure('Rosa.TButton', background=self.colores[3], foreground='white')

        self.root.configure(bg='#f0f0f0')

        # Panel lateral con scroll
        panel_container = ttk.Frame(self.root)
        panel_container.pack(side=tk.LEFT, fill=tk.Y)
        
        # Título fijo
        titulo_frame = ttk.Frame(panel_container, padding=15)
        titulo_frame.pack(side=tk.TOP, fill=tk.X)
        ttk.Label(titulo_frame, text="LOOM ETEC", font=('Helvetica', 18, 'bold')).pack(pady=(0, 10))
        
        # Canvas con scroll para el resto del panel
        canvas_panel = tk.Canvas(panel_container, bg='#f0f0f0', highlightthickness=0, width=280)
        scrollbar_panel = ttk.Scrollbar(panel_container, orient=tk.VERTICAL, command=canvas_panel.yview)
        self.panel = ttk.Frame(canvas_panel, padding=15)
        
        canvas_panel.create_window((0, 0), window=self.panel, anchor='nw')
        canvas_panel.configure(yscrollcommand=scrollbar_panel.set)
        
        canvas_panel.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
        scrollbar_panel.pack(side=tk.RIGHT, fill=tk.Y)
        
        # Actualizar scroll region cuando cambie el contenido
        self.panel.bind('<Configure>', lambda e: canvas_panel.configure(scrollregion=canvas_panel.bbox('all')))
        
        # Scroll con rueda del mouse
        def _on_mousewheel(event):
            canvas_panel.yview_scroll(int(-1*(event.delta/120)), "units")
        canvas_panel.bind_all("<MouseWheel>", _on_mousewheel)

        # Color activo
        ttk.Label(self.panel, text="COLOR ACTIVO", font=('Helvetica', 12, 'bold')).pack(anchor='w')
        self.indicator = tk.Frame(self.panel, width=100, height=40, bg=self.color_seleccionado, relief='sunken', bd=2)
        self.indicator.pack(pady=8)

        frame_colores = ttk.Frame(self.panel)
        frame_colores.pack(pady=5)
        ttk.Button(frame_colores, text="Negro", style='Verde.TButton',
                   command=lambda: self.set_color("#000000")).pack(side=tk.LEFT, padx=5)
        ttk.Button(frame_colores, text="Blanco", style='Naranja.TButton',
                   command=lambda: self.set_color("#FFFFFF")).pack(side=tk.LEFT, padx=5)

        # Pinceles
        ttk.Label(self.panel, text="PINCELES", font=('Helvetica', 12, 'bold')).pack(anchor='w', pady=(20,5))
        self.combo_pinceles = ttk.Combobox(self.panel, values=list(self.pinceles.keys()), state="readonly", width=20)
        self.combo_pinceles.set("Punto")
        self.combo_pinceles.bind("<<ComboboxSelected>>", self.cambiar_pincel)
        self.combo_pinceles.pack(pady=5)

        frame_pincel = ttk.Frame(self.panel)
        frame_pincel.pack(fill=tk.X, pady=5)
        ttk.Button(frame_pincel, text="Crear Pincel", style='Rojo.TButton',
                   command=self.guardar_pincel_custom).pack(side=tk.LEFT, expand=True, fill=tk.X, padx=2)
        ttk.Button(frame_pincel, text="Cargar Pincel", style='Rosa.TButton',
                   command=self.cargar_pincel_custom).pack(side=tk.LEFT, expand=True, fill=tk.X, padx=2)

        # NUEVA SECCIÓN: Edición
        ttk.Label(self.panel, text="EDICIÓN", font=('Helvetica', 12, 'bold')).pack(anchor='w', pady=(20,5))
        ttk.Button(self.panel, text=f"Deshacer ({self.mod_key}+Z)", style='Verde.TButton',
                   command=self.deshacer).pack(fill=tk.X, pady=3)
        ttk.Button(self.panel, text=f"Rehacer ({self.mod_key}+Y)", style='Naranja.TButton',
                   command=self.rehacer).pack(fill=tk.X, pady=3)
        ttk.Button(self.panel, text=f"Limpiar Lienzo ({self.mod_key}+L)", style='Rojo.TButton',
                   command=self.limpiar_lienzo).pack(fill=tk.X, pady=3)

        # Tamaño
        ttk.Label(self.panel, text=f"TAMAÑO ({self.mod_key}+/−)", font=('Helvetica', 12, 'bold')).pack(anchor='w', pady=(20,5))
        ttk.Button(self.panel, text="Agrandar", style='Verde.TButton',
                   command=lambda: self.ajustar_tamano(4)).pack(fill=tk.X, pady=3)
        ttk.Button(self.panel, text="Achicar", style='Naranja.TButton',
                   command=lambda: self.ajustar_tamano(-4)).pack(fill=tk.X, pady=3)

        # Replicar
        ttk.Label(self.panel, text="REPLICAR DISEÑO", font=('Helvetica', 12, 'bold')).pack(anchor='w', pady=(20,5))
        self.spin_filas_rep = ttk.Spinbox(self.panel, from_=1, to=10, width=20)
        self.spin_filas_rep.pack(pady=3)
        ttk.Button(self.panel, text="Aplicar Replicación", style='Rojo.TButton',
                   command=self.replicar_diseno).pack(fill=tk.X, pady=5)

        # Exportar
        ttk.Label(self.panel, text="EXPORTAR", font=('Helvetica', 12, 'bold')).pack(anchor='w', pady=(20,5))
        ttk.Button(self.panel, text=f"Guardar PNG ({self.mod_key}+S)", style='Verde.TButton',
                   command=self.guardar_png).pack(fill=tk.X, pady=3)
        ttk.Button(self.panel, text=f"Guardar JSON ({self.mod_key}+J)", style='Naranja.TButton',
                   command=self.guardar_json).pack(fill=tk.X, pady=3)

        # Canvas principal
        canvas_frame = ttk.Frame(self.root)
        canvas_frame.pack(side=tk.RIGHT, fill=tk.BOTH, expand=True, padx=10, pady=10)

        self.h_scroll = ttk.Scrollbar(canvas_frame, orient=tk.HORIZONTAL)
        self.h_scroll.pack(side=tk.BOTTOM, fill=tk.X)
        self.v_scroll = ttk.Scrollbar(canvas_frame, orient=tk.VERTICAL)
        self.v_scroll.pack(side=tk.RIGHT, fill=tk.Y)

        self.cv = tk.Canvas(canvas_frame, bg="white", highlightthickness=1, highlightbackground='#cccccc',
                           xscrollcommand=self.h_scroll.set, yscrollcommand=self.v_scroll.set)
        self.cv.pack(fill=tk.BOTH, expand=True)

        self.h_scroll.config(command=self.cv.xview)
        self.v_scroll.config(command=self.cv.yview)

        # Eventos de dibujo
        self.cv.bind("<ButtonPress-1>", self.iniciar_pintura)
        self.cv.bind("<B1-Motion>", self.continuar_pintura)
        self.cv.bind("<ButtonRelease-1>", self.terminar_pintura)
        self.cv.bind("<ButtonPress-3>", lambda e: self.iniciar_pintura(e, borrar=True))
        self.cv.bind("<B3-Motion>", lambda e: self.continuar_pintura(e, borrar=True))
        self.cv.bind("<ButtonRelease-3>", self.terminar_pintura)

        # Zoom con Ctrl + rueda
        self.cv.bind("<Control-MouseWheel>", self.zoom_canvas)
        self.cv.bind("<Control-Button-4>", self.zoom_canvas)
        self.cv.bind("<Control-Button-5>", self.zoom_canvas)
        
        # Para Mac
        if self.is_mac:
            self.cv.bind("<Command-MouseWheel>", self.zoom_canvas)

    def zoom_canvas(self, event):
        if event.delta > 0 or event.num == 4:
            if self.size_pixel < 40: self.size_pixel += 2
        elif self.size_pixel > 10:
            self.size_pixel -= 2
        self.actualizar_interfaz()

    def set_color(self, color):
        self.color_seleccionado = color
        self.indicator.config(bg=color)

    def cambiar_pincel(self, event=None):
        self.pincel_actual = self.combo_pinceles.get()

    def actualizar_interfaz(self):
        self.cv.delete("all")
        canvas_width = self.columnas * self.size_pixel + 100
        canvas_height = self.filas * self.size_pixel + 100
        self.cv.config(scrollregion=(0, 0, canvas_width, canvas_height))

        self.matriz_ids = [[None for _ in range(self.columnas)] for _ in range(self.filas)]
        for f in range(self.filas):
            for c in range(self.columnas):
                x1 = c * self.size_pixel + 50
                y1 = f * self.size_pixel + 50
                x2 = x1 + self.size_pixel
                y2 = y1 + self.size_pixel
                rect = self.cv.create_rectangle(x1, y1, x2, y2,
                                                fill=self.matriz_datos[f][c],
                                                outline="#dddddd", width=1)
                self.matriz_ids[f][c] = rect

    def ajustar_tamano(self, cambio):
        nuevo = self.filas + cambio
        if nuevo < self.filas_base:
            messagebox.showwarning("Límite", "Tamaño mínimo alcanzado")
            return
        if cambio > 0:
            for _ in range(cambio):
                self.matriz_datos.append(["#FFFFFF"] * self.columnas)
        else:
            self.matriz_datos = self.matriz_datos[:nuevo]
        self.filas = nuevo
        self.actualizar_interfaz()
        self.guardar_estado()

    def iniciar_pintura(self, event, borrar=False):
        self.pintando = True
        self.ultima_posicion_pincel = None
        self.pintar_con_pincel(event, borrar)

    def continuar_pintura(self, event, borrar=False):
        if self.pintando:
            self.pintar_con_pincel(event, borrar)

    def terminar_pintura(self, event=None):
        if self.pintando:
            self.pintando = False
            self.ultima_posicion_pincel = None
            self.guardar_estado()

    def pintar_con_pincel(self, event, borrar=False):
        x = self.cv.canvasx(event.x)
        y = self.cv.canvasy(event.y)
        centro_c = int((x - 50) // self.size_pixel)
        centro_f = int((y - 50) // self.size_pixel)

        if centro_f < 0 or centro_f >= self.filas or centro_c < 0 or centro_c >= self.columnas:
            return

        pincel = self.pinceles[self.pincel_actual]
        h, w = len(pincel), len(pincel[0]) if pincel else 0
        
        pos_norm_f = centro_f // h
        pos_norm_c = centro_c // w
        
        if self.ultima_posicion_pincel == (pos_norm_f, pos_norm_c):
            return
        
        self.ultima_posicion_pincel = (pos_norm_f, pos_norm_c)
        
        inicio_f = pos_norm_f * h
        inicio_c = pos_norm_c * w

        color = "#FFFFFF" if borrar else self.color_seleccionado
        off_f, off_c = h // 2, w // 2
        
        for pf in range(h):
            for pc in range(w):
                if pincel[pf][pc]:
                    f = inicio_f + pf
                    c = inicio_c + pc
                    if 0 <= f < self.filas and 0 <= c < self.columnas:
                        self.cv.itemconfig(self.matriz_ids[f][c], fill=color)
                        self.matriz_datos[f][c] = color

    def replicar_diseno(self):
        try:
            rep = int(self.spin_filas_rep.get())
        except:
            messagebox.showerror("Error", "Valor inválido")
            return
        if rep <= 1:
            messagebox.showinfo("Info", "No hay cambios")
            return

        original = [fila[:] for fila in self.matriz_datos]
        self.matriz_datos = original * rep
        self.filas *= rep
        self.actualizar_interfaz()
        self.guardar_estado()
        messagebox.showinfo("Éxito", f"Diseño replicado {rep} veces verticalmente")

    def guardar_pincel_custom(self):
        ventana = tk.Toplevel(self.root)
        ventana.title("Crear Pincel Personalizado")
        ventana.geometry("420x600")
        ventana.configure(bg='#f0f0f0')

        ttk.Label(ventana, text="Crear Pincel Personalizado", font=('Helvetica', 14, 'bold')).pack(pady=15)

        frame_size = ttk.Frame(ventana)
        frame_size.pack(pady=10)
        ttk.Label(frame_size, text="Filas:").pack(side=tk.LEFT, padx=10)
        spin_f = ttk.Spinbox(frame_size, from_=1, to=10, width=5)
        spin_f.pack(side=tk.LEFT, padx=5)
        spin_f.delete(0, tk.END); spin_f.insert(0, "3")
        ttk.Label(frame_size, text="Columnas:").pack(side=tk.LEFT, padx=10)
        spin_c = ttk.Spinbox(frame_size, from_=1, to=10, width=5)
        spin_c.pack(side=tk.LEFT, padx=5)
        spin_c.delete(0, tk.END); spin_c.insert(0, "3")

        canvas_pincel = None
        matriz = []
        rects = []

        def generar():
            nonlocal canvas_pincel, matriz, rects
            filas = int(spin_f.get())
            cols = int(spin_c.get())
            if canvas_pincel: canvas_pincel.destroy()
            matriz = [[0] * cols for _ in range(filas)]
            canvas_pincel = tk.Canvas(ventana, width=cols*30+20, height=filas*30+20, bg='white', highlightthickness=1)
            canvas_pincel.pack(pady=20)
            rects = [[None]*cols for _ in range(filas)]
            for f in range(filas):
                for c in range(cols):
                    x1, y1 = c*30 + 10, f*30 + 10
                    rect = canvas_pincel.create_rectangle(x1, y1, x1+30, y1+30, fill='white', outline='gray')
                    rects[f][c] = rect

            def toggle(e):
                cx = (e.x - 10) // 30
                cy = (e.y - 10) // 30
                if 0 <= cy < filas and 0 <= cx < cols:
                    matriz[cy][cx] = 1 - matriz[cy][cx]
                    canvas_pincel.itemconfig(rects[cy][cx], fill='black' if matriz[cy][cx] else 'white')
            canvas_pincel.bind("<Button-1>", toggle)
            canvas_pincel.bind("<B1-Motion>", toggle)

        ttk.Button(frame_size, text="Generar Cuadrícula", command=generar).pack(side=tk.LEFT, padx=20)

        def guardar():
            if not matriz:
                messagebox.showwarning("Error", "Primero genera la cuadrícula")
                return
            nombre = simpledialog.askstring("Nombre", "Nombre del pincel:", parent=ventana)
            if nombre:
                self.pinceles[nombre] = [fila[:] for fila in matriz]
                valores = list(self.combo_pinceles['values'])
                if nombre not in valores: valores.append(nombre)
                self.combo_pinceles['values'] = valores
                self.combo_pinceles.set(nombre)
                self.pincel_actual = nombre

                archivo = filedialog.asksaveasfilename(defaultextension=".brush", filetypes=[("Pincel", "*.brush")])
                if archivo:
                    with open(archivo, 'wb') as f:
                        pickle.dump({'nombre': nombre, 'patron': matriz}, f)
                ventana.destroy()

        ttk.Button(ventana, text="Guardar Pincel", style='Verde.TButton', command=guardar).pack(pady=30)

    def cargar_pincel_custom(self):
        archivo = filedialog.askopenfilename(filetypes=[("Pincel", "*.brush")])
        if archivo:
            try:
                with open(archivo, 'rb') as f:
                    data = pickle.load(f)
                nombre, patron = data['nombre'], data['patron']
                self.pinceles[nombre] = patron
                valores = list(self.combo_pinceles['values'])
                if nombre not in valores: valores.append(nombre)
                self.combo_pinceles['values'] = valores
                self.combo_pinceles.set(nombre)
                self.pincel_actual = nombre
                messagebox.showinfo("Éxito", f"Pincel '{nombre}' cargado")
            except Exception as e:
                messagebox.showerror("Error", str(e))

    def generar_matriz_sincronizada(self):
        fr_f = (self.filas // 4) + 1
        fr_c = (self.columnas // 4) + 1
        tf, tc = self.filas + fr_f, self.columnas + fr_c
        m = [["#FFFFFF"] * tc for _ in range(tf)]
        f_d = 0
        for f in range(tf):
            es_franja_f = f % 5 == 0
            c_d = 0
            for c in range(tc):
                es_franja_c = c % 5 == 0
                if es_franja_f or es_franja_c:
                    m[f][c] = "#FFFFFF" if (f + c) % 2 == 0 else "#000000"
                elif f_d < self.filas and c_d < self.columnas:
                    m[f][c] = self.matriz_datos[f_d][c_d]
                c_d += 1 if not es_franja_c else 0
            f_d += 1 if not es_franja_f else 0
        return m, tf, tc

    def guardar_png(self):
        archivo = filedialog.asksaveasfilename(defaultextension=".png", filetypes=[("PNG", "*.png")])
        if archivo:
            m, h, w = self.generar_matriz_sincronizada()
            img = Image.new("RGB", (w, h))
            for y in range(h):
                for x in range(w):
                    c = m[y][x].lstrip('#')
                    rgb = tuple(int(c[i:i+2], 16) for i in (0,2,4))
                    img.putpixel((x, y), rgb)
            img.save(archivo)
            messagebox.showinfo("Éxito", "PNG guardado")

    def guardar_json(self):
        archivo = filedialog.asksaveasfilename(defaultextension=".json", filetypes=[("JSON", "*.json")])
        if archivo:
            m, h, w = self.generar_matriz_sincronizada()
            binario = [[1 if c == "#000000" else 0 for c in fila] for fila in m]
            json.dump({"dimensiones": [w, h], "datos": binario}, open(archivo, 'w'))
            messagebox.showinfo("Éxito", "JSON guardado")

    def limpiar_lienzo(self):
        if messagebox.askyesno("Confirmar", "¿Limpiar todo el lienzo?"):
            self.filas = self.filas_base
            self.matriz_datos = [["#FFFFFF"] * self.columnas for _ in range(self.filas)]
            self.actualizar_interfaz()
            self.guardar_estado()

if __name__ == "__main__":
    root = tk.Tk()
    app = PixelArtApp(root)
    root.mainloop()