/* =====================================================================
   P15 ESTANQUE DE AGUA + MQTT  ·  Semana 8  ·  v3 (Plantilla Oficial)
   Fundamentos de IoT · 2do semestre 2026
   ---------------------------------------------------------------------
   Basado en la maqueta del curso, adaptado para FSM, YF-S201, HC-SR04 y OLED.
   ===================================================================== */

#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h" // Aquí viven tus contraseñas y tópicos
#define TOPIC "curso/" MQTT_USER "/" PROYECTO "/" NODO  //mandarlo al codigo principal
/* ---------------- 1. CONFIGURACION DE PINES Y TIEMPOS ---------------- */
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0x3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);

#define FLOW_SENSOR_PIN 13
#define RED_LED_PIN     27
#define YELLOW_LED_PIN  4
#define BUZZER_PIN      12
#define BUTTON_PIN      33
#define TRIG_PIN        26
#define ECHO_PIN        25
#define SDA_PIN         21
#define SCL_PIN         22
#define PUMP_PIN        16

const uint32_t PERIODO_LECTURA_MS = 200;    // FSM y Ultrasonido a 5 Hz
const uint32_t PERIODO_FLUJO_MS   = 1000;   // Calculo de caudal a 1 Hz
const uint32_t PERIODO_PUB_MS     = 5000;   // Publicar cada 5 s
const uint32_t REINTENTO_WIFI_MS  = 15000;
const uint32_t ESPERA_INICIAL     = 2000;   // backoff MQTT: 2, 4, 8...
const uint32_t ESPERA_MAXIMA      = 30000;

/* ---------------- 2. ESTADO INTERNO (RED Y FSM) ---------------- */
WiFiClient   red;
PubSubClient mqtt(red);

String clientId, topicDatos, topicEstado, topicCmd;
uint32_t tLectura = 0, tFlujo = 0, tPub = 0, tWiFi = 0, tReconexion = 0;
uint32_t esperaReconexion = ESPERA_INICIAL;

// Variables FSM
enum EstadoFSM { STANDBY, REPOSO, LLENANDO, VACIANDO, DOSIS_COMPLETA, ERROR_SECO };
EstadoFSM estadoActual = STANDBY;

const float UMBRAL_VACIO = 12.0;
const float UMBRAL_LLENO = 4.0;
const float M_CALIBRACION = 1.000;
const float B_CALIBRACION = 0.000;
const float FACTOR_CALIBRACION = 7.5;          
const unsigned long TIMEOUT_BOMBA = 5000;

unsigned long tInicioLlenado = 0;
unsigned long tUltimoBoton = 0;
bool estadoBotonAnterior = HIGH;

// Variables Metrológicas
volatile unsigned long contadorPulsos = 0;
float caudal_mLmin = 0.0;
float distanciaMedida = 999.0;
bool sensorOk = false;

// Dosificación
const float ML_POR_PULSO = 1000.0 / 450.0;     
float volumenTotal_mL = 0.0;                    
float volumenObjetivo_mL = 0.0;                 
bool  dosificacionActiva = false;

/* ---------------- 3. FUNCIONES AUXILIARES ---------------- */

void IRAM_ATTR contarPulsos() { 
  contadorPulsos++; 
}

// 3.1 Comandos entrantes (Telecomando)
void recibirComando(char* topic, byte* payload, unsigned int largo) {
  JsonDocument doc;
  DeserializationError error = deserializeJson(doc, payload, largo);
  if (error) {
    Serial.printf("[cmd] JSON invalido en %s: %s\n", topic, error.c_str());
    return;
  }
  Serial.printf("[cmd] recibido en %s\n", topic);

  if (doc["reset"] == true) {
    volumenTotal_mL = 0.0;
  }
  
  if (doc.containsKey("objetivo_mL")) {
    volumenObjetivo_mL = doc["objetivo_mL"].as<float>();
    dosificacionActiva = (volumenObjetivo_mL > 0);
    volumenTotal_mL = 0.0;                        
  }
  
  if (doc["llenar"] == true &&
      (estadoActual == STANDBY || estadoActual == DOSIS_COMPLETA)) {
    estadoActual = REPOSO;
  }
}

// 3.2 WiFi sin bloquear
void mantenerWiFi() {
  if (WiFi.status() == WL_CONNECTED) return;
  uint32_t ahora = millis();
  if (ahora - tWiFi < REINTENTO_WIFI_MS) return;
  tWiFi = ahora;
  Serial.println("[wifi] sin red, reintentando...");
  WiFi.reconnect();
}

// 3.3 MQTT con testamento y backoff (Adaptado de la maqueta)
void mantenerMQTT() {
  if (mqtt.connected()) return;
  if (WiFi.status() != WL_CONNECTED) return;
  uint32_t ahora = millis();
  if (ahora - tReconexion < esperaReconexion) return;
  tReconexion = ahora;

  Serial.printf("[mqtt] conectando como %s ... ", clientId.c_str());
  if (mqtt.connect(clientId.c_str(), MQTT_USER, MQTT_PASS,
                   topicEstado.c_str(), 1, true, "offline")) {
    Serial.println("OK");
    mqtt.publish(topicEstado.c_str(), "online", true);   
    mqtt.subscribe(topicCmd.c_str(), 1);                 
    esperaReconexion = ESPERA_INICIAL;
  } else {
    Serial.printf("FALLO rc=%d, reintento en %u s\n",
                  mqtt.state(), (unsigned)(esperaReconexion / 1000));
    esperaReconexion = (esperaReconexion * 2 > ESPERA_MAXIMA) ? ESPERA_MAXIMA : esperaReconexion * 2;
  }
}

// 3.4 Publicación JSON Plano
void publicarDatos() {
  if (!mqtt.connected()) return;

  JsonDocument doc;
  
  if (sensorOk) {
    doc["distancia"] = roundf(distanciaMedida * 10.0f) / 10.0f;
    doc["caudal"]    = roundf(caudal_mLmin * 10.0f) / 10.0f;
  }
  doc["volumen_l"]  = volumenTotal_mL / 1000.0;       
  doc["objetivo_l"] = volumenObjetivo_mL / 1000.0;
  doc["estado"]     = (int)estadoActual;
  doc["sensor_ok"]  = sensorOk ? 1 : 0;
  doc["rssi_dbm"]   = WiFi.RSSI();

  char payload[256];
  size_t n = serializeJson(doc, payload, sizeof(payload));

  if (mqtt.publish(topicDatos.c_str(), (const uint8_t*)payload, n, true)) { // Retained = true (Regla del curso para topic principal)
    Serial.printf("[pub] %s -> %s\n", topicDatos.c_str(), payload);
  } else {
    Serial.println("[pub] ERROR publish() (buffer o sesion)");
  }
}

/* ---------------- 4. PROGRAMA PRINCIPAL ---------------- */
void setup() {
  Serial.begin(115200);
  delay(500);                                   
  Serial.println("\n==== P15 ESTANQUE MQTT · Semana 8 ====");

  // Setup de Hardware
  pinMode(BUTTON_PIN, INPUT_PULLUP);
  pinMode(FLOW_SENSOR_PIN, INPUT_PULLUP);
  pinMode(RED_LED_PIN, OUTPUT);
  pinMode(YELLOW_LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(PUMP_PIN, OUTPUT);
  pinMode(TRIG_PIN, OUTPUT);
  pinMode(ECHO_PIN, INPUT);

  digitalWrite(PUMP_PIN, LOW);
  digitalWrite(BUZZER_PIN, LOW);
  digitalWrite(RED_LED_PIN, HIGH);
  digitalWrite(YELLOW_LED_PIN, LOW);

  attachInterrupt(digitalPinToInterrupt(FLOW_SENSOR_PIN), contarPulsos, RISING);

  Wire.begin(SDA_PIN, SCL_PIN);
  display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);

  // Setup de Tópicos
  clientId    = String(MQTT_USER) + "-" + NODO;                               
  topicDatos  = String(TOPIC);  // Extraído de config.h
  topicEstado = topicDatos + "/estado";
  topicCmd    = topicDatos + "/cmd";
  
  Serial.printf("[id] Client ID: %s\n[id] Datos    : %s\n", clientId.c_str(), topicDatos.c_str());

  // Setup WiFi (Espera acotada)
  WiFi.mode(WIFI_STA);
  WiFi.setAutoReconnect(true);
  WiFi.setSleep(false);
  WiFi.begin(WIFI_SSID, WIFI_PASS);
  uint32_t inicio = millis();
  while (WiFi.status() != WL_CONNECTED && millis() - inicio < 10000) {   
    delay(200);
    Serial.print(".");
  }
  Serial.println();
  if (WiFi.status() == WL_CONNECTED) {
    Serial.print("[wifi] IP = ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("[wifi] sin red todavia; el nodo sigue midiendo y reintenta");
  }
  tWiFi = millis();

  // Setup MQTT (Configuraciones robustas de la maqueta)
  mqtt.setServer(MQTT_HOST, MQTT_PORT);
  mqtt.setCallback(recibirComando);
  mqtt.setBufferSize(512);
  mqtt.setKeepAlive(15);       
  mqtt.setSocketTimeout(3);
}

void loop() {
  mantenerWiFi();
  mantenerMQTT();
  mqtt.loop();                 

  uint32_t ahora = millis();

  // --- 4.1 CONTROL MANUAL (Botón) ---
  bool lecturaBoton = digitalRead(BUTTON_PIN);
  if (lecturaBoton == LOW && estadoBotonAnterior == HIGH) {
    if (ahora - tUltimoBoton >= 250) {
      tUltimoBoton = ahora;
      if (estadoActual == STANDBY || estadoActual == ERROR_SECO || estadoActual == DOSIS_COMPLETA) {
        volumenTotal_mL = 0.0;             
        estadoActual = REPOSO;
        digitalWrite(BUZZER_PIN, LOW);
      } else {
        estadoActual = STANDBY;
      }
    }
  }
  estadoBotonAnterior = lecturaBoton;

  // --- 4.2 CAUDALÍMETRO (1 Hz) ---
  if (ahora - tFlujo >= PERIODO_FLUJO_MS) {
    noInterrupts();
    unsigned long pulsos = contadorPulsos;
    contadorPulsos = 0;
    interrupts();

    caudal_mLmin = (((float)pulsos) / FACTOR_CALIBRACION) * 1000.0;
    volumenTotal_mL += pulsos * ML_POR_PULSO;   
    tFlujo = ahora;
  }

  // --- 4.3 FSM, SENSOR HC-SR04 y HMI (5 Hz) ---
  if (ahora - tLectura >= PERIODO_LECTURA_MS) {
    tLectura = ahora;

    digitalWrite(TRIG_PIN, LOW); delayMicroseconds(2);
    digitalWrite(TRIG_PIN, HIGH); delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    
    long duracion = pulseIn(ECHO_PIN, HIGH, 15000); // Timeout rápido
    float distanciaCruda = (duracion == 0) ? 999.0 : (duracion * 0.034 / 2.0);
    distanciaMedida = (distanciaCruda != 999.0) ? (M_CALIBRACION * distanciaCruda) + B_CALIBRACION : 999.0;
    
    sensorOk = (distanciaMedida < 400.0);

    // Máquina de Estados
    switch (estadoActual) {
      case STANDBY:
        digitalWrite(PUMP_PIN, LOW);
        digitalWrite(RED_LED_PIN, HIGH);
        digitalWrite(YELLOW_LED_PIN, LOW);
        break;

      case REPOSO:
        digitalWrite(PUMP_PIN, LOW);
        digitalWrite(RED_LED_PIN, LOW);
        digitalWrite(YELLOW_LED_PIN, HIGH);
        if (sensorOk && distanciaMedida >= UMBRAL_VACIO) {
          estadoActual = LLENANDO;
          tInicioLlenado = ahora;
        }
        break;

      case LLENANDO:
        digitalWrite(PUMP_PIN, HIGH);

        if (dosificacionActiva && volumenTotal_mL >= volumenObjetivo_mL) {
          digitalWrite(PUMP_PIN, LOW);
          estadoActual = DOSIS_COMPLETA;
          break;
        }

        if (sensorOk && distanciaMedida <= UMBRAL_LLENO) {
          estadoActual = VACIANDO;
        } else if (!sensorOk) {
          estadoActual = ERROR_SECO;
        } else if (ahora - tInicioLlenado >= TIMEOUT_BOMBA) {
          if (caudal_mLmin < 50.0) estadoActual = ERROR_SECO;
          else tInicioLlenado = ahora;
        }
        break;

      case VACIANDO:
        digitalWrite(PUMP_PIN, LOW);
        if (sensorOk && distanciaMedida >= UMBRAL_VACIO) {
          estadoActual = LLENANDO;
          tInicioLlenado = ahora;
        }
        break;

      case DOSIS_COMPLETA:
        digitalWrite(PUMP_PIN, LOW);
        digitalWrite(RED_LED_PIN, LOW);
        digitalWrite(YELLOW_LED_PIN, HIGH);
        break;

      case ERROR_SECO:
        digitalWrite(PUMP_PIN, LOW);
        digitalWrite(RED_LED_PIN, HIGH);
        digitalWrite(YELLOW_LED_PIN, LOW);
        digitalWrite(BUZZER_PIN, HIGH);
        break;
    }

    // Pantalla OLED
    display.clearDisplay();
    display.setTextSize(1);
    display.setTextColor(SSD1306_WHITE);

    display.setCursor(0, 0);
    display.print(F("Dist: "));
    if (!sensorOk) display.println(F("ERROR"));
    else { display.print(distanciaMedida, 1); display.println(F(" cm")); }

    display.setCursor(0, 16);
    display.print(F("Q: ")); display.print((int)caudal_mLmin);
    display.println(F(" mL/m"));

    display.setCursor(0, 32);
    display.print(F("Vol: "));
    display.print(volumenTotal_mL / 1000.0, 3);
    display.print(F(" L"));
    if (dosificacionActiva) {
      display.print(F("/"));
      display.print(volumenObjetivo_mL / 1000.0, 2);
    }
    display.println();

    display.setCursor(0, 48);
    display.print(F("FSM: "));
    switch (estadoActual) {
      case STANDBY:        display.println(F("STANDBY")); break;
      case REPOSO:         display.println(F("REPOSO")); break;
      case LLENANDO:       display.println(F("LLENANDO")); break;
      case VACIANDO:       display.println(F("VACIANDO")); break;
      case DOSIS_COMPLETA: display.println(F("DOSIS OK")); break;
      case ERROR_SECO:     display.println(F("ERROR SECO")); break;
    }
    display.display();
  }

  // --- 4.4 PUBLICACIÓN MQTT ---
  if (ahora - tPub >= PERIODO_PUB_MS) {
    publicarDatos();
    tPub = ahora;
  }
}