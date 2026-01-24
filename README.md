# 🧶 LOOM ETEC - Telar Automático con Sistema Jacquard

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](https://opensource.org/licenses/MIT)
[![Platform: ESP32](https://img.shields.io/badge/Platform-ESP32-blue.svg)](https://www.espressif.com/en/products/socs/esp32)
[![Framework: PlatformIO](https://img.shields.io/badge/Framework-PlatformIO-orange.svg)](https://platformio.org/)

## 📖 Descripción del Proyecto

El proyecto **LOOM ETEC** se desarrolla en el marco del taller **AeroGlobETec**, que combina disciplinas como ciencias, electrónica, informática, mecánica y dibujo técnico.

### 🎯 Motivación

Este proyecto nace de una motivación personal, inspirada en la memoria de mi abuela, quien dedicó gran parte de su vida a tejer y donar sus creaciones a quienes más lo necesitaban. Tras su fallecimiento, surgió la idea de conmemorar su legado mediante la creación de un telar automático que perpetúe su espíritu altruista e integre conocimientos técnicos.

**Objetivo principal**: Automatizar el proceso de tejido para producir mantas destinadas a instituciones que asisten a recién nacidos en situación de vulnerabilidad.

### 🏗️ Tipos de Telares Implementados

#### 1️⃣ Telar Convencional

Compuesto por cuatro partes esenciales:

- **Caladores**: Varillas que levantan o bajan selectivamente los hilos de la urdimbre, creando una abertura (llamada "calada") por donde pasa la lanzadera con el hilo de la trama.
- **Peine**: Herramienta dentada que presiona y compacta el hilo de la trama después de cada pasada de la lanzadera.
- **Lanzadera**: Dispositivo que transporta el hilo de la trama a través de los hilos de la urdimbre para tejer la tela.
- **Plegador**: Mecanismo que enrolla la tela tejida, manteniendo la tensión y avanzando la urdimbre a medida que se teje.

#### 2️⃣ Telar Jacquard (31 columnas)

Una reversión del telar de Jacquard, máquina textil que utiliza patrones digitales (en lugar de tarjetas perforadas) para controlar automáticamente el levantamiento individual de hilos de urdimbre, permitiendo la creación de patrones complejos.

**Sistema de control**:
- **Eje X**: Motor PAP para movimiento horizontal (31 posiciones)
- **Eje Z**: Servomotor para selección arriba/abajo de cada hilo
- **Eje Y**: Motor DC para elevación del cabezal (calada)

---

## 🛠️ Tecnologías Utilizadas

### Software

#### **PlatformIO**
IDE profesional para desarrollo de sistemas embebidos. Proporciona gestión de bibliotecas, compilación cruzada y soporte para múltiples plataformas de hardware. Utilizado para programar el ESP32 con C++.

#### **Node-RED**
Herramienta de programación visual basada en flujos para conectar dispositivos IoT. Actúa como middleware entre la interfaz web y el broker MQTT, procesando las peticiones HTTP y transformándolas en comandos MQTT.

#### **MQTT (Mosquitto/HiveMQ Cloud)**
Protocolo de mensajería ligero para IoT. Permite la comunicación asíncrona entre el ESP32 y el servidor mediante topics (tópicos). Se puede usar Mosquitto localmente o HiveMQ Cloud para acceso remoto.

#### **Autodesk Fusion 360**
Software de modelado 3D CAD/CAM utilizado para diseñar las piezas mecánicas del telar, incluyendo soportes, engranajes y estructuras.

#### **Python 3.x + Tkinter**
Lenguaje de programación utilizado para desarrollar el **Editor de Trama**, una aplicación de escritorio que permite crear patrones de tejido de forma visual mediante una interfaz gráfica.

### Hardware

| Componente | Descripción | Cantidad |
|------------|-------------|----------|
| **ESP32 DevKit V1** | Microcontrolador principal con WiFi y Bluetooth integrado | 1 |
| **Motores DC** | Para lanzadera, peine y eje Y (elevación) | 3-4 |
| **Servomotores** | Para selección de hilos (Eje Z) en sistema Jacquard | 1 |
| **Motores PAP + DVR8825** | Motor paso a paso con driver para eje X (horizontal) y plegador | 2 |
| **Puente H L298N** | Controlador para motores DC bidireccionales | 2-3 |
| **Sensores Fin de Carrera** | Detección de posiciones límite (mínimo 7) | 7+ |
| **Fuente de Alimentación** | 12V 5A para motores | 1 |
| **Cables y Conectores** | Jumpers, borneras, conectores Dupont | Varios |

---

## 📋 Requisitos Previos

### Software a Instalar

1. **Visual Studio Code** con extensión **PlatformIO IDE**
2. **Node.js** (v14 o superior) para Node-RED
3. **Python 3.8+** para el Editor de Trama
4. **Mosquitto MQTT Broker** (opcional, para pruebas locales)

---

## 🚀 Guía de Instalación

### 1. Instalación de PlatformIO

1. Descargar e instalar [Visual Studio Code](https://code.visualstudio.com/)
2. Abrir VS Code y dirigirse a **Extensions** (Ctrl+Shift+X)
3. Buscar **PlatformIO IDE** e instalar
4. Reiniciar VS Code
5. Verificar instalación: debería aparecer un ícono de hormiga en la barra lateral

### 2. Instalación de Node-RED

```bash
# Instalar Node.js desde https://nodejs.org/

# Instalar Node-RED globalmente
npm install -g node-red

# Instalar nodos adicionales necesarios
cd ~/.node-red
npm install node-red-dashboard
npm install node-red-contrib-mqtt-broker

# Iniciar Node-RED
node-red

# Acceder a http://localhost:1880
```

### 3. Instalación de Python y Dependencias

```bash
# Verificar instalación de Python
python --version

# Instalar dependencias del Editor de Trama
pip install pillow
pip install tk

# En Linux/Mac, puede ser necesario instalar tkinter:
sudo apt-get install python3-tk  # Ubuntu/Debian
```

### 4. Configuración de MQTT Broker

#### Opción A: Mosquitto Local

```bash
# Windows: Descargar desde https://mosquitto.org/download/
# Linux:
sudo apt-get install mosquitto mosquitto-clients

# Iniciar servicio
sudo systemctl start mosquitto
sudo systemctl enable mosquitto

# Verificar que está corriendo en puerto 1883
netstat -tuln | grep 1883
```

#### Opción B: HiveMQ Cloud (Recomendado para acceso remoto)

1. Crear cuenta gratuita en [HiveMQ Cloud](https://www.hivemq.com/mqtt-cloud-broker/)
2. Crear un nuevo cluster
3. Anotar credenciales (URL, puerto, usuario, contraseña)
4. Actualizar configuración en `main.cpp` y flujos de Node-RED

---

## 🎨 Editor de Trama - Guía de Uso

### Iniciar el Editor

```bash
python Editor-Trama.py
```

### Funcionalidades Principales

#### 1. Crear un Patrón

- **Tamaño**: El editor trabaja con una cuadrícula de 24x24 píxeles por defecto
- **Colores**: 
  - Negro (patrón activo)
  - Blanco (fondo)
- **Pintar**: Click izquierdo para pintar, click derecho para borrar

#### 2. Pinceles Disponibles

| Pincel | Descripción |
|--------|-------------|
| Punto | Pinta un solo píxel |
| Línea H | Línea horizontal de 3 píxeles |
| Línea V | Línea vertical de 3 píxeles |
| Cruz | Forma de cruz (5 píxeles) |
| Cuadrado | Cuadrado de 2x2 |
| Diagonal | Diagonal de 3x3 |

**Crear pincel personalizado**:
1. Click en "Crear Pincel"
2. Definir dimensiones (ej: 3x3)
3. Click en "Generar Cuadrícula"
4. Pintar el patrón deseado
5. "Guardar Pincel" → ingresar nombre → guardar archivo `.brush`

#### 3. Ajustar Tamaño del Lienzo

- **Agrandar**: Ctrl/Cmd + `+` o botón "Agrandar" (incrementa 4 filas)
- **Achicar**: Ctrl/Cmd + `-` o botón "Achicar" (reduce 4 filas, mínimo 24)

#### 4. Replicar Diseño

1. Crear un patrón base en las primeras 24 filas
2. Ingresar número de repeticiones (ej: 3)
3. Click en "Aplicar Replicación"
4. El patrón se repetirá verticalmente

#### 5. Exportar Patrón

##### Guardar como PNG (Ctrl/Cmd + S)
- Genera imagen con franjas de sincronización (patrón ajedrezado)
- Formato: 1 píxel = 1 hilo
- Incluye franjas cada 4 filas/columnas para alineación

##### Guardar como JSON (Ctrl/Cmd + J)
```json
{
  "dimensiones": [31, 100],
  "datos": [
    [0, 1, 0, 1, ...],
    [1, 0, 1, 0, ...],
    ...
  ]
}
```
- **IMPORTANTE**: Para el telar Jacquard, el patrón debe tener exactamente **31 columnas**
- 0 = hilo abajo (blanco), 1 = hilo arriba (negro)

#### 6. Atajos de Teclado

| Atajo | Acción |
|-------|--------|
| Ctrl/Cmd + Z | Deshacer |
| Ctrl/Cmd + Y | Rehacer |
| Ctrl/Cmd + L | Limpiar lienzo |
| Ctrl/Cmd + S | Guardar PNG |
| Ctrl/Cmd + J | Guardar JSON |
| Ctrl/Cmd + `+` | Agrandar lienzo |
| Ctrl/Cmd + `-` | Achicar lienzo |

---

## ⚡ Puesta en Marcha del Telar

### Paso 1: Conexiones del ESP32

#### Telar Convencional

```
LANZADERA:
  enaLanzadera → GPIO 29 (PWM)
  lanzadera1 → GPIO 28 (dirección 1)
  lanzadera2 → GPIO 20 (dirección 2)
  fdcLanzadera1 → GPIO 40 (fin de carrera izq)
  fdcLanzadera2 → GPIO 15 (fin de carrera der)

CALADORES:
  enaCaladores → GPIO 34 (PWM)
  caladores1 → GPIO 35 (subir)
  caladores2 → GPIO 27 (bajar)
  fdcCaladores → GPIO 16 (fin de carrera)

PEINE:
  enaPeine → GPIO 22 (PWM)
  peine1 → GPIO 24 (adelante)
  peine2 → GPIO 23 (atrás)
  fdcPeine1 → GPIO 13 (fin de carrera adelante)
  fdcPeine2 → GPIO 14 (fin de carrera atrás)

PLEGADOR:
  stepPlegador → GPIO 8 (pulsos)
  dirPlegador → GPIO 18 (dirección)
```

#### Telar Jacquard (adicional al convencional)

```
EJE X (Motor PAP):
  stepEjeX → GPIO 25
  dirEjeX → GPIO 26
  fdcEjeX_Home → GPIO 17 (posición inicial)

EJE Z (Servo):
  servoEjeZ → GPIO 19

EJE Y (Motor DC elevación):
  enaEjeY → GPIO 34 (PWM)
  ejeY_Up → GPIO 35
  ejeY_Down → GPIO 27
  fdcEjeY_Arriba → GPIO 16
  fdcEjeY_Abajo → GPIO 21
```

#### Diagrama de Conexión Puente H (ejemplo L298N)

```
ESP32           L298N          Motor DC
GPIO XX   →     ENA      →     (PWM control velocidad)
GPIO YY   →     IN1      →     (dirección)
GPIO ZZ   →     IN2      →     (dirección)
              OUT1/OUT2  →     Motor
GND       →     GND
12V       →     12V (fuente externa)
```

#### Conexión DVR8825 (Motor PAP)

```
ESP32           DVR8825        Motor PAP
GPIO XX   →     STEP
GPIO YY   →     DIR
             →  ENABLE (a GND o control)
12V fuente →    VMOT
GND        →    GND
             →  A1, A2, B1, B2 → Bobinas del motor
```

### Paso 2: Programar el ESP32

```bash
# Clonar el repositorio
git clone https://github.com/tu-usuario/loom-etec.git
cd loom-etec

# Abrir el proyecto en VS Code con PlatformIO
code .

# Configurar WiFi y MQTT en main.cpp (líneas 50-54)
String ssid = "TU_WIFI";
String password = "TU_PASSWORD";
const char* mqtt_server = "TU_BROKER_IP";  # localhost o IP de HiveMQ
const char* mqtt_user = "usuario";
const char* mqtt_pass = "contraseña";

# Compilar y subir (en VS Code)
# Click en el ícono "→" (PlatformIO: Upload) en la barra inferior
# O usar el atajo: Ctrl+Alt+U

# Monitorear salida serial
# Click en el ícono de enchufe (PlatformIO: Serial Monitor)
# O atajo: Ctrl+Alt+S
```

### Paso 3: Configurar Node-RED

```bash
# Iniciar Node-RED
node-red

# Abrir navegador en http://localhost:1880
```

#### Importar Flujo

1. Click en menú hamburguesa (☰) → **Import**
2. Seleccionar archivo:
   - `node-red-flujo-telar.json` (telar Jacquard con carga de patrón)
   - O `node-red-flujo-telar-simple.json` (telar convencional)
3. Click en **Import**

#### Configurar Broker MQTT en Node-RED

1. Doble click en cualquier nodo MQTT (ej: "mqtt_empezar")
2. Click en el lápiz junto a "Server"
3. Configurar:
   ```
   Server: localhost  (o IP del broker)
   Port: 1883
   Username: tu_usuario
   Password: tu_contraseña
   ```
4. Click en **Update** → **Done**
5. Click en **Deploy** (botón rojo arriba a la derecha)

#### Verificar Endpoints HTTP

Una vez desplegado, los siguientes endpoints estarán disponibles:

```
POST http://localhost:1880/api/empezar       - Iniciar tejido
POST http://localhost:1880/api/parar         - Detener tejido
POST http://localhost:1880/api/config/red    - Configurar WiFi
POST http://localhost:1880/api/config/patron - Cargar patrón JSON (solo Jacquard)
GET  http://localhost:1880/api/vueltas       - Obtener contador de vueltas
```

### Paso 4: Interfaz Web

#### Opción 1: Servidor Local Simple

```bash
# Navegar a la carpeta del proyecto
cd loom-etec

# Iniciar servidor HTTP simple con Python
python -m http.server 8000

# Abrir navegador en:
http://localhost:8000/index.html
```

#### Opción 2: Servidor Web Externo

Subir el archivo `index.html` a cualquier hosting web o usar:

```bash
# Con Node.js (live-server)
npm install -g live-server
live-server --port=8000
```

### Paso 5: Operación del Sistema

#### Para Telar Convencional

1. Verificar que el ESP32 esté conectado a WiFi (revisar Serial Monitor)
2. Abrir interfaz web en `http://localhost:8000/index.html`
3. Verificar conexión:
   - Debe mostrar vueltas: 0
   - Botones deben estar habilitados
4. Click en **▶ Iniciar** para comenzar tejido
5. El telar ejecutará la secuencia automáticamente
6. Click en **■ Detener** para pausar

#### Para Telar Jacquard

1. Crear patrón en Editor de Trama (31 columnas)
2. Exportar como JSON (Ctrl+J)
3. Abrir interfaz web
4. En "Cargar patrón":
   - Click en zona de arrastre o seleccionar archivo
   - Elegir el archivo `.json` generado
   - Click en **Subir patrón al telar**
   - Esperar confirmación "🟢 Patrón subido correctamente"
5. Click en **▶ Iniciar**
6. El telar ejecutará el patrón columna por columna

---

## 🔧 Calibración y Ajustes

### Calibración del Sistema Jacquard

En `main.cpp`, ajustar las siguientes constantes según tu hardware:

```cpp
// Línea 60-65
#define PASOS_POR_COLUMNA 200    // Ajustar según distancia entre columnas
#define DELAY_PASO_X 800         // Velocidad del motor PAP (microsegundos)
#define SERVO_POS_ABAJO 45       // Posición servo para dejar hilo abajo
#define SERVO_POS_ARRIBA 135     // Posición servo para fijar hilo arriba
#define SERVO_DELAY 150          // Tiempo de movimiento del servo
```

### Ajuste de Velocidades de Motores

```cpp
// Línea 115-120
ledcWrite(PWM_LANZADERA, 200);  // 0-255: ajustar velocidad lanzadera
ledcWrite(PWM_PEINE, 200);      // 0-255: ajustar velocidad peine
ledcWrite(PWM_EJE_Y, 200);      // 0-255: ajustar velocidad elevación
```

### Tiempos de Estabilización

```cpp
// Línea 70-75
#define DELAY_TEJER 500           // Pausa entre ciclos completos
#define TIEMPO_ESTABILIDAD 200    // Pausa tras cada movimiento
#define TIEMPO_ESPERA_FDC 15000   // Timeout para sensores de fin de carrera
```

---

## 📡 Arquitectura del Sistema

```
┌─────────────────┐
│  Interfaz Web   │ ← Usuario interactúa
│  (index.html)   │
└────────┬────────┘
         │ HTTP (fetch)
         ▼
┌─────────────────┐
│   Node-RED      │ ← Middleware (localhost:1880)
│  (flujo.json)   │
└────────┬────────┘
         │ MQTT
         ▼
┌─────────────────┐
│ Broker MQTT     │ ← Mosquitto/HiveMQ
│  (puerto 1883)  │
└────────┬────────┘
         │ WiFi
         ▼
┌─────────────────┐
│     ESP32       │ ← Control de hardware
│   (main.cpp)    │
└────────┬────────┘
         │ GPIO
         ▼
┌─────────────────┐
│ Motores/Sensores│ ← Sistema mecánico
└─────────────────┘
```

---

## 🐛 Solución de Problemas

### ESP32 no se conecta a WiFi

```cpp
// Verificar credenciales en main.cpp
String ssid = "NOMBRE_EXACTO";  // Sin espacios extra
String password = "PASSWORD_EXACTA";

// Verificar en Serial Monitor:
// - "Conectando a SSID..."
// - "WiFi conectado"
// - "IP: xxx.xxx.xxx.xxx"
```

### Node-RED no recibe mensajes MQTT

1. Verificar broker MQTT está corriendo:
   ```bash
   sudo systemctl status mosquitto
   # o
   mosquitto -v
   ```

2. Probar conexión con mosquitto_sub:
   ```bash
   mosquitto_sub -h localhost -t "empezar" -v
   ```

3. Verificar configuración de broker en Node-RED coincide con ESP32

### Sensores de fin de carrera no funcionan

```cpp
// Verificar lógica de activación (línea 30)
#define FDC_PRESIONADO LOW  // Cambiar a HIGH si usa pull-down

// Agregar resistencias pull-up/down según tipo de sensor
```

### Servo no se mueve correctamente

```cpp
// Calibrar posiciones del servo (línea 62-63)
#define SERVO_POS_ABAJO 45      // Probar valores entre 0-180
#define SERVO_POS_ARRIBA 135    // Ajustar según rango del servo

// Verificar alimentación del servo (5V, GND correcto)
```

### Motor PAP pierde pasos

```cpp
// Aumentar delay entre pasos (línea 61)
#define DELAY_PASO_X 1200  // Valor más alto = movimiento más lento pero preciso

// Verificar:
// - Conexión de bobinas del motor
// - Configuración de micropasos en DVR8825
// - Alimentación suficiente (12V)
```

---

## 📊 Tópicos MQTT

### Publicados por ESP32

| Tópico | Payload | Descripción |
|--------|---------|-------------|
| `vuelta` | Número entero | Contador de vueltas completadas |

### Suscritos por ESP32

| Tópico | Payload | Descripción |
|--------|---------|-------------|
| `empezar` | `"1"` | Iniciar tejido |
| `parar` | `"1"` | Detener tejido |
| `config/red` | JSON: `{"ssid": "...", "password": "..."}` | Configurar WiFi |
| `config/patron` | JSON: `{"filename": "...", "content": "{...}"}` | Cargar patrón (Jacquard) |

---

## 📚 Recursos Adicionales

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

## 📄 Licencia

Este proyecto se desarrolla con fines educativos y sociales en el marco de ETec (Escuela Técnica de la Universidad de Mendoza).

---

## 📧 Contacto

- **Augusto Santoni** - a.santoni@alumno.etec.um.edu.ar
- **Ivo Giovarruscio** - i.giovarruscio@alumno.etec.um.edu.ar

