#include <Arduino.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <PubSubClient.h>
#include <esp_task_wdt.h>

// Pines Lanzadera
#define lanzadera1 23
#define lanzadera2 22
#define fdcLanzadera1 4
#define fdcLanzadera2 2

// Pines Peine
#define peine1 19
#define peine2 21
#define fdcPeine1 15
#define fdcPeine2 13

// Pines Plegador 
#define stepPlegador 16
#define dirPlegador 17

// Pines Motor Calador 
#define caladores1 18
#define caladores2 5
#define fdcCaladores 12  

// Canales PWM
#define PWM_LANZADERA 0
#define PWM_CALADORES 1
#define PWM_PEINE 2

// Configuración
#define CANT_PASOS 150
#define DELAY_PASO 3000 
#define DELAY_TEJER 500
#define FDC_PRESIONADO LOW
#define TIEMPO_ESPERA_FDC 15000
#define TIEMPO_ESTABILIDAD 200

// Tópicos MQTT
#define TOPICO_EMPEZAR "empezar"
#define TOPICO_PARAR "parar"
#define TOPICO_RED "config/red"
#define TOPICO_VUELTA "vuelta"

bool estadoTejiendo = false;
unsigned long vueltas = 0;

String ssid = "PIPO";
String password = "m1lan3sa";
const char* mqtt_server = "192.168.100.153";
const char* mqtt_user = "chopo.mqtt";
const char* mqtt_pass = "m1lan3sa";
#define PUERTO_MQTT 1883

WiFiClient espClient;
PubSubClient client(espClient);

void parar() {
  estadoTejiendo = false;
  Serial.println("Parando el telar...");
  
  // Apagar todos los motores
  digitalWrite(lanzadera1, LOW);
  digitalWrite(lanzadera2, LOW);
  digitalWrite(caladores1, LOW);
  digitalWrite(caladores2, LOW);
  digitalWrite(peine1, LOW);
  digitalWrite(peine2, LOW);
  
  ledcWrite(PWM_LANZADERA, 0);
  ledcWrite(PWM_CALADORES, 0);
  ledcWrite(PWM_PEINE, 0);
}

bool verificarFDC(int fdcPin) {
  unsigned long t0 = millis();
  Serial.print("Esperando FDC en pin: ");
  Serial.println(fdcPin);
  
  while (digitalRead(fdcPin) != FDC_PRESIONADO) {
    if (millis() - t0 > TIEMPO_ESPERA_FDC) {
      Serial.println("FDC no se activó, saliendo del bucle. Pin: " + String(fdcPin));
      parar();
      return false;
    }
    if (!estadoTejiendo) return false;
    client.loop();
  }
  
  Serial.println("FDC activado");
  return true;
}

void setup_wifi() {
  delay(10);
  Serial.println();
  Serial.print("Conectando a ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid.c_str(), password.c_str());

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());
  Serial.println("");
  Serial.println("WiFi conectado");
  Serial.println("IP: ");
  Serial.println(WiFi.localIP());
}

void reconnect() {
  while (!client.connected()) {
    Serial.print("Intentando conexión MQTT...");
    String clientId = "ESP32Client-";
    clientId += String(random(0xffff), HEX);
    
    if (client.connect(clientId.c_str(), mqtt_user, mqtt_pass)) {
      Serial.println("conectado");
      client.subscribe(TOPICO_PARAR);
      client.subscribe(TOPICO_EMPEZAR);
      client.subscribe(TOPICO_RED);
    } else {
      Serial.print("falló, rc=");
      Serial.print(client.state());
      Serial.println(" reintentando en 5 segundos");
      delay(5000);
    }
  }
}

void callback(char* topic, byte* payload, unsigned int length) {
  Serial.print("Mensaje recibido [");
  Serial.print(topic);
  Serial.print("]: ");
  
  if (strcmp(topic, TOPICO_EMPEZAR) == 0 && payload[0] == '1') {
    estadoTejiendo = true;
    Serial.println("Iniciando telar");
  } else if (strcmp(topic, TOPICO_PARAR) == 0 && payload[0] == '1') {
    parar();
  } else if (strcmp(topic, TOPICO_RED) == 0) {
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

void tejer() {
  // Paso 1: Caladores (posición 1)
  digitalWrite(caladores1, HIGH);
  digitalWrite(caladores2, LOW);
  ledcWrite(PWM_CALADORES, 200);
  if (!verificarFDC(fdcCaladores)) return;
  ledcWrite(PWM_CALADORES, 0);
  digitalWrite(caladores1, LOW);
  delay(TIEMPO_ESTABILIDAD);
  client.loop();

  // Paso 2: Lanzadera ida
  digitalWrite(lanzadera1, HIGH);
  digitalWrite(lanzadera2, LOW);
  ledcWrite(PWM_LANZADERA, 200);
  if (!verificarFDC(fdcLanzadera1)) return;
  ledcWrite(PWM_LANZADERA, 0);
  digitalWrite(lanzadera1, LOW);
  delay(TIEMPO_ESTABILIDAD);
  client.loop();

  // Paso 3: Peine adelante
  digitalWrite(peine1, HIGH);
  digitalWrite(peine2, LOW);
  ledcWrite(PWM_PEINE, 200);
  if (!verificarFDC(fdcPeine1)) return;
  ledcWrite(PWM_PEINE, 0);
  digitalWrite(peine1, LOW);
  delay(TIEMPO_ESTABILIDAD);
  client.loop();

  // Paso 4: Peine atrás
  digitalWrite(peine1, LOW);
  digitalWrite(peine2, HIGH);
  ledcWrite(PWM_PEINE, 200);
  if (!verificarFDC(fdcPeine2)) return;
  ledcWrite(PWM_PEINE, 0);
  digitalWrite(peine2, LOW);
  delay(TIEMPO_ESTABILIDAD);
  client.loop();

  // Paso 5: Plegador
  digitalWrite(dirPlegador, HIGH);
  for (int i = 0; i < CANT_PASOS; i++) {
    digitalWrite(stepPlegador, HIGH);
    delayMicroseconds(DELAY_PASO);
    digitalWrite(stepPlegador, LOW);
    delayMicroseconds(DELAY_PASO);
  }
  client.loop();

  // Paso 6: Caladores (posición 2)
  digitalWrite(caladores1, LOW);
  digitalWrite(caladores2, HIGH);
  ledcWrite(PWM_CALADORES, 200);
  if (!verificarFDC(fdcCaladores)) return;
  ledcWrite(PWM_CALADORES, 0);
  digitalWrite(caladores2, LOW);
  delay(TIEMPO_ESTABILIDAD);
  client.loop();

  // Paso 7: Lanzadera vuelta
  digitalWrite(lanzadera1, LOW);
  digitalWrite(lanzadera2, HIGH);
  ledcWrite(PWM_LANZADERA, 200);
  if (!verificarFDC(fdcLanzadera2)) return;
  ledcWrite(PWM_LANZADERA, 0);
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
  delay(TIEMPO_ESTABILIDAD);
  client.loop();

  // Paso 9: Peine atrás
  digitalWrite(peine1, LOW);
  digitalWrite(peine2, HIGH);
  ledcWrite(PWM_PEINE, 200);
  if (!verificarFDC(fdcPeine2)) return;
  ledcWrite(PWM_PEINE, 0);
  digitalWrite(peine2, LOW);
  delay(TIEMPO_ESTABILIDAD);
  client.loop();

  // Paso 10: Plegador
  digitalWrite(dirPlegador, HIGH);
  for (int i = 0; i < CANT_PASOS; i++) {
    digitalWrite(stepPlegador, HIGH);
    delayMicroseconds(DELAY_PASO);
    digitalWrite(stepPlegador, LOW);
    delayMicroseconds(DELAY_PASO);
  }
  client.loop();

  vueltas++;
  Serial.print("Vuelta completada: ");
  Serial.println(vueltas);
  client.publish(TOPICO_VUELTA, String(vueltas).c_str());
}

void setup() {
  pinMode(lanzadera1, OUTPUT);
  pinMode(lanzadera2, OUTPUT);
  pinMode(fdcLanzadera1, INPUT_PULLUP);
  pinMode(fdcLanzadera2, INPUT_PULLUP);
  ledcSetup(PWM_LANZADERA, 5000, 8);

  pinMode(caladores1, OUTPUT);
  pinMode(caladores2, OUTPUT);
  pinMode(fdcCaladores, INPUT_PULLUP);
  ledcSetup(PWM_CALADORES, 5000, 8);

  pinMode(peine1, OUTPUT);
  pinMode(peine2, OUTPUT);
  pinMode(fdcPeine1, INPUT_PULLUP);
  pinMode(fdcPeine2, INPUT_PULLUP);
  ledcSetup(PWM_PEINE, 5000, 8);

  pinMode(stepPlegador, OUTPUT);
  pinMode(dirPlegador, OUTPUT);

  Serial.begin(115200);
  
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
  delay(100);
}