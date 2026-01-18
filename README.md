# LOOMETEC: Telar Automático

**Proyecto desarrollado en ETec** por Augusto Santoni e Ivo Giovarruscio  
Docente a cargo: Patricia Elizabeth Furci

---

## 📋 Tabla de Contenidos

- [Descripción del Proyecto](#descripción-del-proyecto)
- [Motivación](#motivación)
- [Componentes del Sistema](#componentes-del-sistema)
- [Instalación y Configuración](#instalación-y-configuración)
- [Editor de Trama (Python)](#editor-de-trama-python)
- [Hardware y Conexiones ESP32](#hardware-y-conexiones-esp32)
- [Impresión 3D](#impresión-3d)
- [Uso del Sistema](#uso-del-sistema)
- [Recursos Adicionales](#recursos-adicionales)

---

## 🎯 Descripción del Proyecto

LoomETec es un telar automático desarrollado en el marco del taller AeroGlobETec, que integra disciplinas como ciencias, electrónica, informática, mecánica y dibujo técnico. El sistema automatiza el proceso de tejido para producir mantas destinadas a instituciones que asisten a recién nacidos en situación de vulnerabilidad.

### Componentes Principales del Telar

1. **Caladores**: Varillas que levantan/bajan los hilos de urdimbre
2. **Peine**: Compacta el hilo de trama después de cada pasada
3. **Lanzadera**: Transporta el hilo de trama
4. **Plegador**: Enrolla la tela tejida manteniendo la tensión

---

## 💝 Motivación

Este proyecto nace como homenaje a mi abuela, quien dedicó su vida a tejer y donar sus creaciones. Tras su fallecimiento, surgió la idea de perpetuar su legado mediante un telar automático que combine tecnología e impacto social.

---

## 🔧 Componentes del Sistema

### Software
- **PlatformIO** (Visual Studio Code)
- **Node-RED** (control y flujos)
- **HiveMQ Cloud** (broker MQTT)
- **Autodesk Fusion 360** (diseño 3D)
- **Python 3.x** (Editor de Trama)

### Hardware
- **ESP32 DevKit V1** (microcontrolador principal)
- **Motores DC** con sensores fin de carrera
- **Motores PAP (Paso a Paso)** con driver DVR8825
- **Puente H** (driver para motores DC)
- **Sensores de fin de carrera**

---

## 💻 Instalación y Configuración

### 1. Entorno de Desarrollo (PlatformIO)

\`\`\`bash
# Instalar Visual Studio Code
# Luego instalar la extensión PlatformIO

# Clonar el repositorio
git clone https://github.com/augustoSantoni/LoomETec.git
cd LoomETec

# Abrir el proyecto en VS Code
code .
\`\`\`

### 2. Configuración del ESP32

El archivo \`platformio.ini\` ya está configurado:

\`\`\`ini
[env:esp32doit-devkit-v1]
platform = espressif32
board = esp32doit-devkit-v1
framework = arduino

lib_deps = 
    knolleary/PubSubClient@^2.8
    bblanchon/ArduinoJson@^7.4.2
monitor_speed = 115200
\`\`\`

### 3. Dependencias de Python (Editor de Trama)

\`\`\`bash
# Instalar dependencias
pip install tkinter pillow

# En macOS, tkinter viene preinstalado con Python
# En Linux:
sudo apt-get install python3-tk python3-pil python3-pil.imagetk
\`\`\`

---

## 🎨 Editor de Trama (Python)

### Descripción

El **Editor de Trama** es una aplicación gráfica que permite diseñar patrones de tejido pixel por pixel. Genera archivos PNG y JSON que pueden ser interpretados por el sistema del telar.

### Características Principales

- **Lienzo de 24x24 píxeles** (expandible)
- **Sistema de pinceles personalizados**
- **Deshacer/Rehacer** (hasta 50 acciones)
- **Exportación PNG y JSON**
- **Replicación de patrones**
- **Atajos de teclado**

### Uso del Editor

\`\`\`bash
# Ejecutar el editor
cd TelarSimple  # o la carpeta donde esté Editor-Trama.py
python3 Editor-Trama.py
\`\`\`

#### Controles Principales

**Atajos de Teclado:**
- \`Cmd/Ctrl + Z\`: Deshacer
- \`Cmd/Ctrl + Y\`: Rehacer
- \`Cmd/Ctrl + L\`: Limpiar lienzo
- \`Cmd/Ctrl + Plus\`: Agrandar lienzo
- \`Cmd/Ctrl + Minus\`: Reducir lienzo
- \`Cmd/Ctrl + S\`: Guardar PNG
- \`Cmd/Ctrl + J\`: Guardar JSON

**Ratón:**
- **Clic izquierdo**: Dibujar con color seleccionado
- **Clic derecho**: Borrar (pintar de blanco)
- **Ctrl + Rueda**: Zoom del canvas

#### Pinceles Predeterminados

1. **Punto**: Pixel individual
2. **Línea H**: 3 píxeles horizontales
3. **Línea V**: 3 píxeles verticales
4. **Cruz**: Patrón en cruz
5. **Cuadrado**: 2x2 píxeles
6. **Diagonal**: Línea diagonal 3x3

#### Crear Pinceles Personalizados

1. Clic en **"Crear Pincel"**
2. Definir dimensiones (filas y columnas)
3. Clic en **"Generar Cuadrícula"**
4. Clic en los cuadros para activar/desactivar píxeles
5. **"Guardar Pincel"** y asignar nombre
6. Se guarda como archivo \`.brush\` para reutilizar

#### Replicar Diseño

Para crear patrones repetitivos:
1. Diseña un patrón base
2. Ingresa número de repeticiones
3. Clic en **"Aplicar Replicación"**
4. El diseño se repite verticalmente

#### Exportar Diseños

**PNG** (visualización):
- Genera una imagen con franjas de sincronización
- Cada 5 píxeles se inserta una franja blanco/negro
- Útil para verificar el patrón

**JSON** (para el telar):
- Matriz binaria: 1 = negro, 0 = blanco
- Incluye dimensiones del patrón
- Compatible con el sistema de control

\`\`\`json
{
  "dimensiones": [29, 29],
  "datos": [
    [0, 1, 0, 1, ...],
    [1, 0, 1, 0, ...],
    ...
  ]
}
\`\`\`

---

## ⚡ Hardware y Conexiones ESP32

### Lista de Materiales

| Componente | Cantidad | Descripción |
|------------|----------|-------------|
| ESP32 DevKit V1 | 1 | Microcontrolador principal |
| Motor DC | 2-4 | Para lanzadera y plegador |
| Motor PAP NEMA 17 | 1-2 | Para caladores |
| Driver DVR8825 | 1-2 | Control de motores PAP |
| Puente H L298N | 1-2 | Control de motores DC |
| Fin de carrera | 4-6 | Sensores de posición |
| Fuente 12V 5A | 1 | Alimentación motores |
| Cables Dupont | - | Conexiones |

### Diagrama de Conexiones

#### Motor PAP (Caladores)

\`\`\`
ESP32          DVR8825
GPIO 25   -->  STEP
GPIO 26   -->  DIR
GPIO 27   -->  ENABLE
              
              Motor PAP
DVR8825    -->  NEMA 17
A+/A-      -->  Bobina A
B+/B-      -->  Bobina B

Fuente 12V -->  VMOT/GND (DVR8825)
\`\`\`

#### Motores DC (Lanzadera/Plegador)

\`\`\`
ESP32          L298N (Puente H)
GPIO 18   -->  IN1
GPIO 19   -->  IN2
GPIO 21   -->  ENA (PWM)

L298N     -->  Motor DC
OUT1/OUT2 -->  Motor 1
OUT3/OUT4 -->  Motor 2

Fuente 12V -->  12V/GND (L298N)
\`\`\`

#### Sensores Fin de Carrera

\`\`\`
ESP32          Fin de Carrera
GPIO 32   -->  Fin carrera 1 (COM)
GPIO 33   -->  Fin carrera 2 (COM)
GPIO 34   -->  Fin carrera 3 (COM)
GPIO 35   -->  Fin carrera 4 (COM)

GND       -->  NO/NC (según configuración)
\`\`\`

### Configuración de Pines (Ejemplo)

\`\`\`cpp
// Pines Motores PAP
#define STEP_PIN 25
#define DIR_PIN 26
#define ENABLE_PIN 27

// Pines Motores DC
#define MOTOR_DC1_IN1 18
#define MOTOR_DC1_IN2 19
#define MOTOR_DC1_ENA 21

// Pines Fin de Carrera
#define FIN_CARRERA_1 32
#define FIN_CARRERA_2 33
#define FIN_CARRERA_3 34
#define FIN_CARRERA_4 35
\`\`\`

### Alimentación

⚠️ **IMPORTANTE:**
- ESP32: 5V vía USB o VIN
- Motores: 12V fuente externa
- **NO** conectar motores directo al ESP32
- Usar **fuente común (GND compartido)** entre ESP32 y drivers

---

## 🖨️ Impresión 3D

### Archivos STL

Los modelos 3D se encuentran en la carpeta del proyecto:
- Componentes mecánicos del telar
- Soportes para sensores
- Carcasas para electrónica

### Parámetros de Impresión Recomendados

\`\`\`
Material: PLA o PETG
Altura de capa: 0.2mm
Relleno: 20-30%
Soportes: Según pieza
Velocidad: 50-60 mm/s
Temperatura: 200-210°C (PLA)
Cama: 60°C
\`\`\`

### Piezas Críticas

- **Caladores**: Requieren precisión dimensional
- **Peine**: Imprimir con mayor relleno (40%)
- **Soportes de motores**: PETG recomendado (mayor resistencia)

### Post-Procesado

1. Eliminar soportes cuidadosamente
2. Lijar zonas de contacto con lija 220
3. Verificar ajuste con componentes electrónicos
4. Opcional: acetona para suavizar superficies (ABS)

---

## 🚀 Uso del Sistema

### 1. Preparación

\`\`\`bash
# Compilar y subir código al ESP32
pio run -t upload

# Iniciar Node-RED
node-red

# Abrir navegador en http://localhost:1880
\`\`\`

### 2. Diseño del Patrón

1. Ejecutar \`Editor-Trama.py\`
2. Crear diseño
3. Exportar JSON
4. Cargar JSON en Node-RED

### 3. Control del Telar

1. Verificar conexión MQTT
2. Cargar patrón desde interfaz
3. Iniciar ciclo de tejido
4. Monitorear proceso

---

## 📚 Recursos Adicionales

### Enlaces del Proyecto

- [Código Final](https://github.com/augustoSantoni/LoomETec)
- Animación Telar Digital
- Videos Sistema Electrónico

### Documentación Técnica

- [Datasheet ESP32](https://www.espressif.com/sites/default/files/documentation/esp32_datasheet_en.pdf)
- [PlatformIO Docs](https://docs.platformio.org/)
- [Node-RED Guide](https://nodered.org/docs/)

### Referencias

1. R. Jander's personal website - https://r.jander.me.uk/
2. Textalks YouTube channel - https://www.youtube.com/@textalks
3. Picturing Homeric Weaving - https://chs.harvard.edu/
4. LEGO Mindstorms NXT Loom - https://youtu.be/IPIJsdvDjsc

---

## 🔮 Mejoras Futuras

- **Caladores más complejos** para patrones avanzados
- **Interfaz web** con Paho MQTT
- **Control independiente** por hilo
- **Sistema de reconocimiento** de errores
- **Base de datos** de patrones

---

## 📄 Licencia

Este proyecto se desarrolla con fines educativos y sociales en el marco de ETec.

---

## 🙏 Agradecimientos

Este proyecto está dedicado a la memoria de mi abuela, cuyo legado de amor y generosidad inspiró esta creación.

**Agradecimientos especiales:**
- ETec y taller AeroGlobETec
- Profesora Patricia Furci
- Ivo Giovarruscio (co-desarrollador)
- Comunidad maker y open source

---

## 📧 Contacto

- **Augusto Santoni** - a.santoni@alumno.etec.um.edu.ar
- **Ivo Giovarruscio** - i.giovarruscio@alumno.etec.um.edu.ar

---

*"Tejiendo tecnología con propósito social"* 🧶🤖
