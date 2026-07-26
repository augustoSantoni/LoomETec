#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <ESP32Servo.h>
#include <esp_task_wdt.h>

// ============================================
// DEFINICIÓN DE PINES - SISTEMA JACQUARD
// ============================================

// Lanzadera (Motor DC con puente H)
#define lanzadera1    23
#define lanzadera2    22
#define enaLanzadera  34    // Pin PWM enable del puente H de la lanzadera
#define fdcLanzadera1  4    // FDC lado izquierdo
#define fdcLanzadera2  2    // FDC lado derecho

// Peine (Motor DC con puente H)
#define peine1        19
#define peine2        21
#define enaPeine      35    // Pin PWM enable del puente H del peine
#define fdcPeine1     15    // FDC peine adelante
#define fdcPeine2     13    // FDC peine atrás

// Plegador (Motor paso a paso con driver A4988/DRV8825)
#define stepPlegador  16
#define dirPlegador   17

// Calador - Eje Y (Motor DC con puente H)
#define ejeY_Up       18    // Dirección subir
#define ejeY_Down      5    // Dirección bajar
#define enaEjeY       36    // Pin PWM enable del puente H del calador  ← NUEVO
#define fdcEjeY_Arriba 12   // FDC posición superior  (antes fdcEjeY1)
#define fdcEjeY_Abajo  25   // FDC posición inferior  (antes fdcEjeY2)

// Eje X - Motor 28BYJ-48 (motor unipolar de 4 fases, sin driver externo)
#define stepX_Pin1    26
#define stepX_Pin2    27
#define stepX_Pin3    33
#define stepX_Pin4    32
// FDC de referencia / home del eje X
#define fdcEjeX_Home  14    // ← reasignado (servoEjeZ se mueve a pin 39)

// Servo eje Z (pistón Jacquard)
#define servoEjeZ_Pin 39    // ← reasignado para liberar pin 14

// ============================================
// CANALES PWM  (ledcSetup / ledcAttachPin)
// ============================================

#define PWM_LANZADERA  0
#define PWM_PEINE      2
#define PWM_EJE_Y      4

// ============================================
// CONFIGURACIÓN DEL SISTEMA JACQUARD
// ============================================

#define NUM_COLUMNAS       31
// Secuencia de pasos del 28BYJ-48 (half-step, 8 fases)
// Da mayor torque y suavidad que full-step
#define PASOS_POR_COLUMNA  512   // pasos half-step entre columnas
#define DELAY_PASO_X_US    1200  // µs entre cada fase del 28BYJ-48

#define SERVO_POS_ABAJO    45    // pistón fuera (hilo queda abajo)
#define SERVO_POS_ARRIBA   135   // pistón dentro (hilo sube)
#define SERVO_DELAY_MS     150   // tiempo para que el servo se mueva

// ============================================
// CONFIGURACIÓN GENERAL
// ============================================

#define CANT_PASOS_PLEGADOR  150
#define DELAY_PASO_PLEGADOR  3000   // µs entre pasos del plegador
#define DELAY_TEJER_MS       500
#define FDC_PRESIONADO       LOW
#define TIEMPO_ESPERA_FDC    15000  // ms máximo esperando un FDC
#define TIEMPO_ESTABILIDAD   200    // ms de pausa tras alcanzar FDC

// Tópicos MQTT
#define TOPICO_EMPEZAR  "empezar"
#define TOPICO_PARAR    "parar"
#define TOPICO_RED      "config/red"
#define TOPICO_VUELTA   "vuelta"
#define TOPICO_PATRON   "config/patron"

// ============================================
// VARIABLES GLOBALES
// ============================================

bool estadoTejiendo = false;
unsigned long vueltas = 0;

String ssid     = "PIPO";
String password = "m1lan3sa";
const char* mqtt_server = "192.168.100.153";
const char* mqtt_user   = "chopo.mqtt";
const char* mqtt_pass   = "m1lan3sa";
#define PUERTO_MQTT 1883

WiFiClient espClient;
PubSubClient client(espClient);
Servo servoZ;

#define MAX_FILAS 500
uint8_t patron[MAX_FILAS][NUM_COLUMNAS];
int numFilas    = 0;
int filaActual  = 0;
bool patronCargado = false;

// Posición lógica actual del eje X (columna 0 … NUM_COLUMNAS-1)
int posicionEjeX = 0;

// Tabla de secuencia half-step para 28BYJ-48
// 4 bobinas, 8 pasos por ciclo
const uint8_t halfStep[8][4] = {
  {1, 0, 0, 0},
  {1, 1, 0, 0},
  {0, 1, 0, 0},
  {0, 1, 1, 0},
  {0, 0, 1, 0},
  {0, 0, 1, 1},
  {0, 0, 0, 1},
  {1, 0, 0, 1}
};

// ============================================
// FUNCIONES DE CONTROL BÁSICO
// ============================================

void parar() {
  estadoTejiendo = false;
  Serial.println("Parando el telar...");

  digitalWrite(lanzadera1, LOW);
  digitalWrite(lanzadera2, LOW);
  digitalWrite(peine1,     LOW);
  digitalWrite(peine2,     LOW);
  digitalWrite(ejeY_Up,    LOW);
  digitalWrite(ejeY_Down,  LOW);

  ledcWrite(PWM_LANZADERA, 0);
  ledcWrite(PWM_PEINE,     0);
  ledcWrite(PWM_EJE_Y,     0);

  // Apagar bobinas del 28BYJ-48 para no calentar
  digitalWrite(stepX_Pin1, LOW);
  digitalWrite(stepX_Pin2, LOW);
  digitalWrite(stepX_Pin3, LOW);
  digitalWrite(stepX_Pin4, LOW);
}

/**
 * Espera hasta que el pin FDC quede en FDC_PRESIONADO.
 * Devuelve false si se supera el timeout o se para el tejido.
 */
bool verificarFDC(int fdcPin) {
  unsigned long t0 = millis();
  Serial.print("Esperando FDC en pin ");
  Serial.println(fdcPin);

  while (digitalRead(fdcPin) != FDC_PRESIONADO) {
    if (millis() - t0 > TIEMPO_ESPERA_FDC) {
      Serial.print("TIMEOUT: FDC no se activó en pin ");
      Serial.println(fdcPin);
      parar();
      return false;
    }
    if (!estadoTejiendo) return false;
    client.loop();
  }

  Serial.println("FDC activado OK");
  return true;
}

// ============================================
// FUNCIONES DEL EJE X (28BYJ-48)
// ============================================

/**
 * Aplica una fase de la secuencia half-step al motor 28BYJ-48.
 * paso: índice 0–7 de la tabla halfStep[]
 */
void aplicarPaso28BYJ(int paso) {
  paso = ((paso % 8) + 8) % 8;  // wrap seguro
  digitalWrite(stepX_Pin1, halfStep[paso][0]);
  digitalWrite(stepX_Pin2, halfStep[paso][1]);
  digitalWrite(stepX_Pin3, halfStep[paso][2]);
  digitalWrite(stepX_Pin4, halfStep[paso][3]);
  delayMicroseconds(DELAY_PASO_X_US);
}

/**
 * Lleva el eje X al home buscando el FDC de referencia.
 * Mueve en sentido antihorario (índice decreciente) hasta activar el FDC.
 */
void homeEjeX() {
  Serial.println("HOME Eje X...");

  int paso = 0;
  unsigned long t0 = millis();

  while (digitalRead(fdcEjeX_Home) != FDC_PRESIONADO) {
    paso = (paso + 7) % 8;  // retroceder una fase (antihorario)
    aplicarPaso28BYJ(paso);

    if (millis() - t0 > TIEMPO_ESPERA_FDC) {
      Serial.println("TIMEOUT: Home Eje X fallido");
      parar();
      return;
    }
    if (!estadoTejiendo) return;
  }

  // Apagar bobinas al llegar
  digitalWrite(stepX_Pin1, LOW);
  digitalWrite(stepX_Pin2, LOW);
  digitalWrite(stepX_Pin3, LOW);
  digitalWrite(stepX_Pin4, LOW);

  posicionEjeX = 0;
  delay(TIEMPO_ESTABILIDAD);
  Serial.println("Eje X en HOME (columna 0)");
}

/**
 * Mueve el eje X a la columna indicada (0 … NUM_COLUMNAS-1).
 * Siempre hace home primero para garantizar posición absoluta.
 */
void moverEjeX(int columna) {
  if (columna < 0 || columna >= NUM_COLUMNAS) return;

  homeEjeX();
  if (!estadoTejiendo) return;
  if (columna == 0) return;

  int totalPasos = columna * PASOS_POR_COLUMNA;
  int paso = 0;

  for (int i = 0; i < totalPasos; i++) {
    paso = (paso + 1) % 8;  // avanzar (horario)
    aplicarPaso28BYJ(paso);
    if (!estadoTejiendo) return;
  }

  // Apagar bobinas al terminar
  digitalWrite(stepX_Pin1, LOW);
  digitalWrite(stepX_Pin2, LOW);
  digitalWrite(stepX_Pin3, LOW);
  digitalWrite(stepX_Pin4, LOW);

  posicionEjeX = columna;
  delay(TIEMPO_ESTABILIDAD);
}

// ============================================
// FUNCIÓN DEL SERVO Z (PISTÓN JACQUARD)
// ============================================

/**
 * valor == 1 → hilo SUBE  → servo esconde pistón (ARRIBA)
 * valor == 0 → hilo BAJA  → servo introduce pistón (ABAJO)
 */
void configurarServoZ(uint8_t valor) {
  int posicion = (valor == 1) ? SERVO_POS_ARRIBA : SERVO_POS_ABAJO;
  servoZ.write(posicion);
  delay(SERVO_DELAY_MS);
}

// ============================================
// FUNCIONES DEL CALADOR (EJE Y)
// ============================================

/**
 * Sube el calador hasta el FDC superior.
 */
bool subirCalador() {
  Serial.println("Subiendo calador...");

  digitalWrite(ejeY_Up,   HIGH);
  digitalWrite(ejeY_Down, LOW);
  ledcWrite(PWM_EJE_Y, 200);

  if (!verificarFDC(fdcEjeY_Arriba)) return false;

  ledcWrite(PWM_EJE_Y, 0);
  digitalWrite(ejeY_Up,   LOW);
  digitalWrite(ejeY_Down, LOW);
  delay(TIEMPO_ESTABILIDAD);

  Serial.println("Calador arriba OK");
  return true;
}

/**
 * Baja el calador hasta el FDC inferior.
 */
bool bajarCalador() {
  Serial.println("Bajando calador...");

  digitalWrite(ejeY_Up,   LOW);
  digitalWrite(ejeY_Down, HIGH);
  ledcWrite(PWM_EJE_Y, 200);

  if (!verificarFDC(fdcEjeY_Abajo)) return false;

  ledcWrite(PWM_EJE_Y, 0);
  digitalWrite(ejeY_Up,   LOW);
  digitalWrite(ejeY_Down, LOW);
  delay(TIEMPO_ESTABILIDAD);

  Serial.println("Calador abajo OK");
  return true;
}

// ============================================
// CONFIGURACIÓN DE LA CALADA (pasos 3-6)
// ============================================

/**
 * Configura la calada para la fila indicada del patrón.
 *
 * Secuencia interna:
 *   3. Eje X se alinea con cada hilo (columna 0 a 30) usando 28BYJ-48.
 *   4. Servo Z sube o baja el pistón según el valor del patrón.
 *   5. Se repite para las 31 columnas.
 *   6. Motor DC sube los hilos seleccionados hasta el FDC superior.
 *
 * Al finalizar el tejido de la pasada, el caller debe llamar a bajarCalador().
 */
bool configurarCalada(int fila) {
  if (!patronCargado || fila < 0 || fila >= numFilas) {
    Serial.println("ERROR: Patrón no cargado o fila fuera de rango");
    parar();
    return false;
  }

  Serial.print("Configurando calada – fila ");
  Serial.println(fila);

  // PASO 3+4+5: posicionar servo en cada columna
  for (int col = 0; col < NUM_COLUMNAS; col++) {
    if (!estadoTejiendo) return false;

    moverEjeX(col);
    if (!estadoTejiendo) return false;

    uint8_t valor = patron[fila][col];
    configurarServoZ(valor);

    Serial.print(valor);
    if (col < NUM_COLUMNAS - 1) Serial.print(",");

    client.loop();
  }
  Serial.println();

  // Volver al home del eje X antes de subir el calador
  homeEjeX();
  if (!estadoTejiendo) return false;

  // PASO 6: subir calador (hilos seleccionados suben)
  if (!subirCalador()) return false;

  return true;
}

// ============================================
// PROCESAMIENTO DE PATRÓN JSON
// ============================================

bool procesarPatronJSON(const char* jsonString) {
  DynamicJsonDocument doc(8192);

  DeserializationError error = deserializeJson(doc, jsonString);
  if (error) {
    Serial.print("Error parseando JSON: ");
    Serial.println(error.c_str());
    return false;
  }

  if (!doc.containsKey("dimensiones") || !doc.containsKey("datos")) {
    Serial.println("JSON inválido: faltan 'dimensiones' o 'datos'");
    return false;
  }

  JsonArray dimensiones = doc["dimensiones"];
  int cols  = dimensiones[0];
  int filas = dimensiones[1];

  if (cols != NUM_COLUMNAS) {
    Serial.printf("ERROR: patrón con %d columnas, se esperan %d\n", cols, NUM_COLUMNAS);
    return false;
  }

  if (filas > MAX_FILAS) {
    Serial.printf("ERROR: patrón con %d filas, máximo %d\n", filas, MAX_FILAS);
    return false;
  }

  JsonArray datos = doc["datos"];
  numFilas = 0;

  for (JsonArray fila : datos) {
    if (numFilas >= MAX_FILAS) break;
    int col = 0;
    for (int valor : fila) {
      if (col >= NUM_COLUMNAS) break;
      patron[numFilas][col] = (valor != 0) ? 1 : 0;
      col++;
    }
    numFilas++;
  }

  Serial.printf("Patrón cargado: %d filas x %d columnas\n", numFilas, NUM_COLUMNAS);
  filaActual = 0;
  patronCargado = true;
  return true;
}

// ============================================
// FUNCIÓN PRINCIPAL DE TEJIDO
// Secuencia completa de 1 ciclo (2 pasadas de lanzadera)
// ============================================

void tejer() {
  if (!patronCargado) {
    Serial.println("ERROR: No hay patrón cargado.");
    parar();
    return;
  }

  // ─────────────────────────────────────────
  // ══ PRIMERA PASADA ══
  // ─────────────────────────────────────────
  Serial.printf("=== PRIMERA PASADA – fila %d/%d ===\n", filaActual, numFilas);

  // PASOS 3-6: configurar calada (mueve eje X, servo Z, sube calador)
  if (!configurarCalada(filaActual)) return;
  client.loop();

  // PASO 7: Lanzadera de izquierda a derecha → espera FDC lado derecho
  Serial.println("Lanzadera → derecha");
  digitalWrite(lanzadera1, HIGH);
  digitalWrite(lanzadera2, LOW);
  ledcWrite(PWM_LANZADERA, 200);
  if (!verificarFDC(fdcLanzadera2)) return;   // FDC del lado destino
  ledcWrite(PWM_LANZADERA, 0);
  digitalWrite(lanzadera1, LOW);
  digitalWrite(lanzadera2, LOW);
  delay(TIEMPO_ESTABILIDAD);
  client.loop();

  // PASO 8a: Peine adelante (aprieta el hilo)
  Serial.println("Peine adelante");
  digitalWrite(peine1, HIGH);
  digitalWrite(peine2, LOW);
  ledcWrite(PWM_PEINE, 200);
  if (!verificarFDC(fdcPeine1)) return;
  ledcWrite(PWM_PEINE, 0);
  digitalWrite(peine1, LOW);
  digitalWrite(peine2, LOW);
  delay(TIEMPO_ESTABILIDAD);
  client.loop();

  // PASO 8b: Peine atrás (vuelve a posición inicial)
  Serial.println("Peine atrás");
  digitalWrite(peine1, LOW);
  digitalWrite(peine2, HIGH);
  ledcWrite(PWM_PEINE, 200);
  if (!verificarFDC(fdcPeine2)) return;
  ledcWrite(PWM_PEINE, 0);
  digitalWrite(peine1, LOW);
  digitalWrite(peine2, LOW);
  delay(TIEMPO_ESTABILIDAD);
  client.loop();

  // PASO 9: Bajar calador (los hilos seleccionados vuelven abajo)
  if (!bajarCalador()) return;
  client.loop();

  // PASO 10: Plegador avanza un incremento
  Serial.println("Plegador");
  digitalWrite(dirPlegador, HIGH);
  for (int i = 0; i < CANT_PASOS_PLEGADOR; i++) {
    digitalWrite(stepPlegador, HIGH);
    delayMicroseconds(DELAY_PASO_PLEGADOR);
    digitalWrite(stepPlegador, LOW);
    delayMicroseconds(DELAY_PASO_PLEGADOR);
  }
  client.loop();

  // Avanzar a la siguiente fila del patrón
  filaActual++;
  if (filaActual >= numFilas) {
    Serial.println("Patrón completado → reiniciando desde fila 0");
    filaActual = 0;
  }

  // ─────────────────────────────────────────
  // ══ SEGUNDA PASADA ══
  // ─────────────────────────────────────────
  Serial.printf("=== SEGUNDA PASADA – fila %d/%d ===\n", filaActual, numFilas);

  // PASOS 3-6: nueva calada para la siguiente fila
  if (!configurarCalada(filaActual)) return;
  client.loop();

  // PASO 7: Lanzadera de derecha a izquierda → espera FDC lado izquierdo
  Serial.println("Lanzadera → izquierda");
  digitalWrite(lanzadera1, LOW);
  digitalWrite(lanzadera2, HIGH);
  ledcWrite(PWM_LANZADERA, 200);
  if (!verificarFDC(fdcLanzadera1)) return;   // FDC del lado destino
  ledcWrite(PWM_LANZADERA, 0);
  digitalWrite(lanzadera1, LOW);
  digitalWrite(lanzadera2, LOW);
  delay(TIEMPO_ESTABILIDAD);
  client.loop();

  // PASO 8a: Peine adelante
  Serial.println("Peine adelante");
  digitalWrite(peine1, HIGH);
  digitalWrite(peine2, LOW);
  ledcWrite(PWM_PEINE, 200);
  if (!verificarFDC(fdcPeine1)) return;
  ledcWrite(PWM_PEINE, 0);
  digitalWrite(peine1, LOW);
  digitalWrite(peine2, LOW);
  delay(TIEMPO_ESTABILIDAD);
  client.loop();

  // PASO 8b: Peine atrás
  Serial.println("Peine atrás");
  digitalWrite(peine1, LOW);
  digitalWrite(peine2, HIGH);
  ledcWrite(PWM_PEINE, 200);
  if (!verificarFDC(fdcPeine2)) return;
  ledcWrite(PWM_PEINE, 0);
  digitalWrite(peine1, LOW);
  digitalWrite(peine2, LOW);
  delay(TIEMPO_ESTABILIDAD);
  client.loop();

  // PASO 9: Bajar calador
  if (!bajarCalador()) return;
  client.loop();

  // PASO 10: Plegador
  Serial.println("Plegador");
  digitalWrite(dirPlegador, HIGH);
  for (int i = 0; i < CANT_PASOS_PLEGADOR; i++) {
    digitalWrite(stepPlegador, HIGH);
    delayMicroseconds(DELAY_PASO_PLEGADOR);
    digitalWrite(stepPlegador, LOW);
    delayMicroseconds(DELAY_PASO_PLEGADOR);
  }
  client.loop();

  // ─────────────────────────────────────────
  Serial.println("=== FIN DE CICLO COMPLETO ===");
  vueltas++;
  client.publish(TOPICO_VUELTA, String(vueltas).c_str());

  // Avanzar a la siguiente fila para el próximo ciclo
  filaActual++;
  if (filaActual >= numFilas) {
    Serial.println("Patrón completado → reiniciando desde fila 0");
    filaActual = 0;
  }
}

// ============================================
// WIFI Y MQTT
// ============================================

void setup_wifi() {
  delay(10);
  Serial.print("Conectando a ");
  Serial.println(ssid);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.print("\nWiFi conectado – IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Conectando MQTT...");
    String clientId = "ESP32Client-" + String(random(0xffff), HEX);
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("conectado");
      client.subscribe(TOPICO_PARAR);
      client.subscribe(TOPICO_EMPEZAR);
      client.subscribe(TOPICO_RED);
      client.subscribe(TOPICO_PATRON);
    } else {
      Serial.printf("falló rc=%d, reintentando en 5s\n", client.state());
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensaje [");
  Serial.print(topic);
  Serial.print("]: ");

  char mensaje[length + 1];
  memcpy(mensaje, payload, length);
  mensaje[length] = '\0';
  Serial.println(mensaje);

  if (strcmp(topic, TOPICO_EMPEZAR) == 0 && payload[0] == '1') {
    if (patronCargado) {
      estadoTejiendo = true;
      Serial.println("Iniciando tejido...");
    } else {
      Serial.println("ERROR: No se puede iniciar sin patrón cargado");
    }
  }
  else if (strcmp(topic, TOPICO_PARAR) == 0 && payload[0] == '1') {
    parar();
  }
  else if (strcmp(topic, TOPICO_RED) == 0) {
    StaticJsonDocument<256> doc;
    if (!deserializeJson(doc, payload)) {
      ssid     = doc["ssid"].as<String>();
      password = doc["password"].as<String>();
      Serial.println("Nueva red recibida, reconectando...");
      setup_wifi();
      reconnect();
    }
  }
  else if (strcmp(topic, TOPICO_PATRON) == 0) {
    Serial.println("Recibiendo patrón JSON...");
    StaticJsonDocument<512> docWrapper;
    if (deserializeJson(docWrapper, payload)) {
      Serial.println("Error parseando wrapper del patrón");
      return;
    }
    const char* content = docWrapper["content"];
    if (content == nullptr) {
      Serial.println("ERROR: falta campo 'content'");
      return;
    }
    if (procesarPatronJSON(content)) {
      Serial.println("✓ Patrón cargado exitosamente");
      filaActual = 0;
    } else {
      Serial.println("✗ Error al cargar el patrón");
    }
  }
}

// ============================================
// SETUP
// ============================================

void setup() {
  Serial.begin(115200);
  Serial.println("\n=== TELAR JACQUARD ESP32 v3.0 ===");

  // --- Lanzadera ---
  pinMode(lanzadera1,    OUTPUT);
  pinMode(lanzadera2,    OUTPUT);
  pinMode(fdcLanzadera1, INPUT_PULLUP);
  pinMode(fdcLanzadera2, INPUT_PULLUP);
  ledcSetup(PWM_LANZADERA, 5000, 8);
  ledcAttachPin(enaLanzadera, PWM_LANZADERA);

  // --- Peine ---
  pinMode(peine1,    OUTPUT);
  pinMode(peine2,    OUTPUT);
  pinMode(fdcPeine1, INPUT_PULLUP);
  pinMode(fdcPeine2, INPUT_PULLUP);
  ledcSetup(PWM_PEINE, 5000, 8);
  ledcAttachPin(enaPeine, PWM_PEINE);

  // --- Plegador ---
  pinMode(stepPlegador, OUTPUT);
  pinMode(dirPlegador,  OUTPUT);

  // --- Eje X (28BYJ-48) ---
  pinMode(stepX_Pin1,   OUTPUT);
  pinMode(stepX_Pin2,   OUTPUT);
  pinMode(stepX_Pin3,   OUTPUT);
  pinMode(stepX_Pin4,   OUTPUT);
  pinMode(fdcEjeX_Home, INPUT_PULLUP);
  // Apagar bobinas al inicio
  digitalWrite(stepX_Pin1, LOW);
  digitalWrite(stepX_Pin2, LOW);
  digitalWrite(stepX_Pin3, LOW);
  digitalWrite(stepX_Pin4, LOW);

  // --- Servo Z ---
  servoZ.attach(servoEjeZ_Pin);
  servoZ.write(SERVO_POS_ABAJO);

  // --- Calador (Eje Y) ---
  pinMode(ejeY_Up,        OUTPUT);
  pinMode(ejeY_Down,      OUTPUT);
  pinMode(fdcEjeY_Arriba, INPUT_PULLUP);
  pinMode(fdcEjeY_Abajo,  INPUT_PULLUP);
  ledcSetup(PWM_EJE_Y, 5000, 8);
  ledcAttachPin(enaEjeY, PWM_EJE_Y);

  // --- WiFi y MQTT ---
  setup_wifi();
  client.setServer(mqtt_server, PUERTO_MQTT);
  client.setCallback(callback);
  client.setBufferSize(16384);

  // --- Home inicial ---
  estadoTejiendo = true;   // habilitamos movimiento solo para el home
  homeEjeX();
  estadoTejiendo = false;

  // Asegurarse de que el calador esté abajo al arrancar
  // (sin esto podría arrancar a mitad de recorrido)
  estadoTejiendo = true;
  bajarCalador();
  estadoTejiendo = false;

  Serial.println("Sistema listo. Esperando patrón vía MQTT...");
}

// ============================================
// LOOP
// ============================================

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (estadoTejiendo) {
    delay(DELAY_TEJER_MS);
    tejer();
  }

  delay(100);
}