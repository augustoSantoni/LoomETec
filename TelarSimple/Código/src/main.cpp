#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiClient.h> // Cambiado de WiFiClientSecure a WiFiClient
#include <PubSubClient.h>
#include <esp_task_wdt.h>

// Hay que comprobar las direcciones de los motores
// No usar 1, 6, 7, 8, 11, 24, 28, 29, 30, 31
// Lanzadera
#define enaLanzadera 29
#define lanzadera1 28
#define lanzadera2 20
#define fdcLanzadera1 40
#define fdcLanzadera2 15

// Caladores
#define enaCaladores 34
#define caladores1 35
#define caladores2 27
#define fdcCaladores 16

// Peine
#define enaPeine 22
#define peine1 24
#define peine2 23
#define fdcPeine1 13 //adelante
#define fdcPeine2 14 //atras

// Plegador
#define stepPlegador 8 //lo cambie de 25 a 26
#define dirPlegador 18

// Canales PWM
#define PWM_LANZADERA 0
#define PWM_CALADORES 1
#define PWM_PEINE 2
#define PWM_TENSION 3

// Configuración
#define CANT_PASOS 150 // ajustar?
#define DELAY_PASO 3000 // ajustar? (microsegundos)
#define DELAY_TEJER 500 // ajustar?
#define FDC_PRESIONADO LOW // Depende del FDC si cuando se presiona devuelve HIGH o LOW
#define TIEMPO_ESPERA_FDC 15000 // Tiempo de espera hasta que el FDC se activa, por si nunca toca y no se activa el FDC, ajustar?
#define TIEMPO_ESTABILIDAD 200 // Tiempo de estabilidad, ajustar?

// Topicos MQTT
#define TOPICO_EMPEZAR "empezar" // Topico para la contraseña del WiFi
#define TOPICO_PARAR "parar" // Topico para parar el telar
#define TOPICO_RED "config/red" // Topico para la configuración
#define TOPICO_VUELTA "vuelta"

bool estadoTejiendo = false; // Variable para saber si se está tejiendo o no

unsigned long vueltas = 0; // Contador de vueltas
String ssid = "PIPO";
String password = "m1lan3sa";
const char* mqtt_server = "192.168.100.153"; // IP del servidor Mosquitto
const char* mqtt_user = "chopo.mqtt"; // Usuario de Mosquitto
const char* mqtt_pass = "m1lan3sa"; // Contraseña de Mosquitto

#define PUERTO_MQTT 1883 // Puerto estándar de Mosquitto (sin SSL)

WiFiClient espClient; // Cambiado de WiFiClientSecure a WiFiClient
PubSubClient client(espClient);

void parar(){
  estadoTejiendo = false; // Cambiar el estado de tejiendo a false
  Serial.println("Parando el telar...");
  // Detener todos los motores
  digitalWrite(lanzadera1, LOW);
  digitalWrite(lanzadera2, LOW);
  digitalWrite(caladores1, LOW);
  digitalWrite(caladores2, LOW);
  digitalWrite(peine1, LOW);
  digitalWrite(peine2, LOW);
  ledcWrite(PWM_LANZADERA, 0);
  ledcWrite(PWM_CALADORES, 0);
  ledcWrite(PWM_PEINE, 0);
  ledcWrite(PWM_TENSION, 0);
  return;
}

bool verificarFDC(int fdcPin) {
  unsigned long t0 = millis(); // Inicializar el tiempo de espera
  Serial.print("Esperando FDC en pin: ");
  Serial.println(fdcPin);
  while (digitalRead(fdcPin) != FDC_PRESIONADO){
    if (millis() - t0 > TIEMPO_ESPERA_FDC) { // Si el FDC no se activa en el tiempo esperado
      Serial.println("FDC no se activó, saliendo del bucle. Pin: " + String(fdcPin));
      parar(); // Llamar a la función parar si el FDC no se activa
      return false; // False si el FDC no se activa en el tiempo esperado
    }
    if (!estadoTejiendo) return false; // Si se detiene el telar, salir del bucle
    client.loop(); // Mantener el cliente MQTT activo
  };
  Serial.println("");
  return true; // Si el FDC se activa correctamente
}

void setup_wifi() {
  delay(10);
  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("connected");
      client.subscribe(TOPICO_PARAR);
      client.subscribe(TOPICO_EMPEZAR);
      client.subscribe(TOPICO_RED);
    } else {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(1000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("]: ");
  if (strcmp(topic, TOPICO_EMPEZAR) == 0 && payload[0] == '1') estadoTejiendo = true;
  else
  {
    if (strcmp(topic, TOPICO_PARAR) == 0 && payload[0] == '1') parar();
    else {
      if (strcmp(topic, TOPICO_RED) == 0) {
        StaticJsonDocument<256> doc;
        DeserializationError error = deserializeJson(doc, payload);
        if (!error) {
          ssid = doc["ssid"].as<String>();
          password = doc["password"].as<String>();
          Serial.println("Recibido SSID y PASSWORD, conectando...");
          setup_wifi();
          reconnect();
        } else {
          Serial.println("Error al parsear JSON");
        }
      }
    }
  }
}

void tejer() {
  // Paso 1: Caladores (primero siempre)
  digitalWrite(caladores1, HIGH);
  digitalWrite(caladores2, LOW);
  ledcWrite(PWM_CALADORES, 200);
  if (!verificarFDC(fdcCaladores)) return; // Verificamos el FDC para los caladores
  ledcWrite(PWM_CALADORES, 0);
  digitalWrite(caladores1, LOW);
  digitalWrite(caladores2, LOW);
  delay(TIEMPO_ESTABILIDAD);
  client.loop();

  // Paso 2: Lanzadera 1
  digitalWrite(lanzadera1, HIGH);
  digitalWrite(lanzadera2, LOW);
  ledcWrite(PWM_LANZADERA, 200);
  if (!verificarFDC(fdcLanzadera1)) return;
  ledcWrite(PWM_LANZADERA, 0);
  digitalWrite(lanzadera1, LOW);
  digitalWrite(lanzadera2, LOW);
  delay(TIEMPO_ESTABILIDAD);
  client.loop();

  // Paso 3: Peine adelante
  digitalWrite(peine1, HIGH);
  digitalWrite(peine2, LOW);
  ledcWrite(PWM_PEINE, 200);
  if (!verificarFDC(fdcPeine1)) return;
  ledcWrite(PWM_PEINE, 0);
  digitalWrite(peine1, LOW);
  digitalWrite(peine2, LOW);
  delay(TIEMPO_ESTABILIDAD);
  client.loop();

  // Paso 4: Peine atrás
  digitalWrite(peine1, LOW);
  digitalWrite(peine2, HIGH);
  ledcWrite(PWM_PEINE, 200);
  if (!verificarFDC(fdcPeine2)) return;
  ledcWrite(PWM_PEINE, 0);
  digitalWrite(peine1, LOW);
  digitalWrite(peine2, LOW);
  delay(TIEMPO_ESTABILIDAD);
  client.loop();

  // Paso 5: Plegador
  digitalWrite(dirPlegador, HIGH);
  digitalWrite(stepPlegador, LOW);
  for (int i = 0; i < CANT_PASOS; i++) {
    digitalWrite(stepPlegador, HIGH);
    delayMicroseconds(DELAY_PASO);
    digitalWrite(stepPlegador, LOW);
    delayMicroseconds(DELAY_PASO);
  }
  client.loop();
  Serial.println("Fin vuelta");

  // Paso 6: Repetir Caladores (fdcCal)
  digitalWrite(caladores1, HIGH);
  digitalWrite(caladores2, LOW);
  ledcWrite(PWM_CALADORES, 200);
  if (!verificarFDC(fdcCaladores)) return;
  ledcWrite(PWM_CALADORES, 0);
  digitalWrite(caladores1, LOW);
  digitalWrite(caladores2, LOW);
  delay(TIEMPO_ESTABILIDAD);
  client.loop();

  // Paso 7: Lanzadera 2
  digitalWrite(lanzadera1, LOW);
  digitalWrite(lanzadera2, HIGH);
  ledcWrite(PWM_LANZADERA, 200);
  if (!verificarFDC(fdcLanzadera2)) return;
  ledcWrite(PWM_LANZADERA, 0);
  digitalWrite(lanzadera1, LOW);
  digitalWrite(lanzadera2, LOW);
  delay(TIEMPO_ESTABILIDAD);
  client.loop();

  // Paso 8: Peine adelante
  digitalWrite(peine1, HIGH);
  digitalWrite(peine2, LOW);
  ledcWrite(PWM_PEINE, 200);
  if (!verificarFDC(fdcPeine1)) return;
  ledcWrite(PWM_PEINE, 0);
  digitalWrite(peine1, LOW);
  digitalWrite(peine2, LOW);
  delay(TIEMPO_ESTABILIDAD);
  client.loop();

  // Paso 9: Peine atrás
  digitalWrite(peine1, LOW);
  digitalWrite(peine2, HIGH);
  ledcWrite(PWM_PEINE, 200);
  if (!verificarFDC(fdcPeine2)) return;
  ledcWrite(PWM_PEINE, 0);
  digitalWrite(peine1, LOW);
  digitalWrite(peine2, LOW);
  delay(TIEMPO_ESTABILIDAD);
  client.loop();

  // Paso 10: Plegador
  digitalWrite(dirPlegador, HIGH);
  digitalWrite(stepPlegador, LOW);
  for (int i = 0; i < CANT_PASOS; i++) {
    digitalWrite(stepPlegador, HIGH);
    delayMicroseconds(DELAY_PASO);
    digitalWrite(stepPlegador, LOW);
    delayMicroseconds(DELAY_PASO);
  }
  client.loop();
  Serial.println("Fin vuelta");
  vueltas++;
  client.publish(TOPICO_VUELTA, String(vueltas).c_str());
}

void setup() {
  pinMode(lanzadera1, OUTPUT);
  pinMode(lanzadera2, OUTPUT);
  pinMode(fdcLanzadera1, INPUT);
  pinMode(fdcLanzadera2, INPUT);
  ledcAttachPin(enaLanzadera, PWM_LANZADERA);
  ledcSetup(PWM_LANZADERA, 5000, 8);

  pinMode(caladores1, OUTPUT);
  pinMode(caladores2, OUTPUT);
  pinMode(fdcCaladores, INPUT);
  ledcAttachPin(enaCaladores, PWM_CALADORES);
  ledcSetup(PWM_CALADORES, 5000, 8);

  pinMode(peine1, OUTPUT);
  pinMode(peine2, OUTPUT);
  pinMode(fdcPeine1, INPUT);
  pinMode(fdcPeine2, INPUT);
  ledcAttachPin(enaPeine, PWM_PEINE);
  ledcSetup(PWM_PEINE, 5000, 8);

  pinMode(stepPlegador, OUTPUT);
  pinMode(dirPlegador, OUTPUT);

  pinMode(BUILTIN_LED, OUTPUT);
  Serial.begin(115200);
  // Ya no se necesita el certificado para conexión sin SSL
  setup_wifi();
  client.setServer(mqtt_server, PUERTO_MQTT);
  client.setCallback(callback);
}

void loop() {
  if (!client.connected()) {
    reconnect();
  }
  client.loop();

  if (estadoTejiendo) {
    delay(DELAY_TEJER);
    tejer();
  }
  delay(1000);
}