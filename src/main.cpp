// Proyecto y Diseño Electrónico 2025
// Instituto Don Orione
// Prof. Juan Carlos Torres

// Libreria para soporte Wi-Fi
#include <WiFi.h>
// MQTT
#include <PubSubClient.h>
// Libreria para soporte Json version 7
#include <ArduinoJson.h>
// DHT22
#include <DHTesp.h>

// Publicacion
const char* topic_pub = "ido/esp32";
// Subscripcion
const char* topic_sub = "ido/kodular";

// Credenciales Wi-Fi
const char* ssid = "Wokwi-GUEST";
const char* password = "";

// Broker MQTT
const char* mqtt_server = "broker.hivemq.com";

// Matriz de leds basada en Max7219
#include <MD_Parola.h>
#include <MD_MAX72xx.h>
#include <SPI.h>
#define HARDWARE_TYPE MD_MAX72XX::PAROLA_HW
#define MAX_DEVICES 4
#define CS_PIN 5
// Conexion SPI
MD_Parola Pantalla = MD_Parola (HARDWARE_TYPE, CS_PIN, MAX_DEVICES);

// Pulsadores
#define PULSADOR_LED1 22
#define PULSADOR_LED2 4
#define PULSADOR_RESET 15

// Leds
#define LED1 32
#define LED2 27

// Configuración del DHT22
#define DHT_PIN 13
#define DHT_TYPE DHT22
// DHT dht (DHT_PIN, DHTT_YPE);
DHTesp dht;

// Variables globales
bool EstadoP1 = false;
bool EstadoP2 = false;
bool EstadoReset = false;
String Mensaje = "-";
bool EstadoLed1 = false;
bool EstadoLed2 = false;
unsigned long UltimaLectura = 0;
const long Intervalo = 2000; // para lectura DHT
float Temperatura;
float Humedad;

WiFiClient espClient;
PubSubClient clienteMQTT (espClient);

void configurarWiFi () {
  delay(100);
  Serial.print ("Conectando a red Wi-Fi: ");
  Serial.print (ssid);
  WiFi.begin (ssid, password);
  while (WiFi.status () != WL_CONNECTED) {
    delay (300);
    Serial.print (".");
  }
  randomSeed (micros());
  Serial.println ("");
  Serial.print ("Conexion exitosa, ");
  Serial.print ("IP: ");
  Serial.println (WiFi.localIP());
}
void publicarMQTT ()
{
  // envia actulizacion de estado por MQTT
  StaticJsonDocument<300> JSONdoc;
  JsonObject JSONencoder = JSONdoc.to<JsonObject>();

  JSONencoder["relay1"] = EstadoLed1;
  JSONencoder["relay2"] = EstadoLed2;
  JSONencoder["led"] = Mensaje;

  char JSONmessageBuffer[100];
  serializeJson (JSONdoc, JSONmessageBuffer);
  clienteMQTT.publish (topic_pub, JSONmessageBuffer);
  Serial.print ("Enviando JSON: ");
  Serial.println (JSONmessageBuffer);
  Serial.println ("=======================");
}

void recibirMQTT (char* topic, byte* payload, unsigned int length)
{
  // Recibe mensaje del topico suscrito
  Serial.print ("Mensaje recibido en topico: ");
  Serial.println (topic);
  String json_received;
  Serial.print ("JSON Recibido: ");
  for (int i = 0; i < length; i++) {
    json_received += ((char)payload[i]);
    //Serial.print((char)payload[i]);
  }
  Serial.println (json_received);
  // en caso de recibir el comando 'status', se envia actualizacion
  if (json_received == "status")
      publicarMQTT ();
  else
  {
    // Parsear json
    JsonDocument root;
    deserializeJson(root, json_received);

    //get json parsed value
    //sample of json: {"device":"relay1","status":true}
    //sample of json: {"device":"led","text":"Text display"}
    Serial.print ("Commando: ");
    String device = root["device"];
    String trigger;
    // LED1
    if (device == "relay1") {
      if (root["status"] == true) {
        // encender
        digitalWrite (LED1, HIGH);
        trigger = "ON";
        EstadoLed1 = true;
      }
      else {
        // apagar
        digitalWrite (LED1, LOW);
        trigger = "OFF";
        EstadoLed1 = false;
      }
      Serial.println ("Estado " + device + ": " + trigger);
    }
    // LED2
    if (device == "relay2") {
      if (root["status"] == true)
      {
        // encender
        digitalWrite (LED2, HIGH);
        trigger = "ON";
        EstadoLed2 = true;
      }
      else {
        digitalWrite (LED2, HIGH);
        trigger = "OFF";
        EstadoLed2 = false;
      }
      Serial.println("Estado " + device + ": " + trigger);
    }
    // Pantalla
    if (device == "led") {
      // Mostrar texto
      Mensaje = root["text"].as<const char*>();
      Serial.println ("Texto a mostrar: " + Mensaje);
    }
    // Publicar estado
    publicarMQTT();
  }
}

void reconectarMQTT () {
  // Esperar a reconexion
  while (!clienteMQTT.connected()) {
    Serial.println ("Intentando conexion MQTT... ");
    // genero un ID aleatorio
    String clientId = "IDO-";
    clientId += String (random(0xffff), HEX);

    // if (client.connect(clientId,userName,passWord))
    if (clienteMQTT.connect (clientId.c_str())) {
      clienteMQTT.subscribe (topic_sub); // subscripcion a topico
      Serial.print ("Conectado a: ");
      Serial.println (mqtt_server);
      Serial.println ("Cliente MQTT generado: " + clientId);
      Serial.print ("Suscrito a topico: ");
      Serial.println (topic_sub);
    } else {
      Serial.print ("Error = ");
      Serial.print (clienteMQTT.state());
      Serial.println (", reintentando en 5 seg...");
      delay (5000);
    }
  }
}

void setup() {
  
  Serial.begin (115200);
  Pantalla.begin (); // matriz leds
  dht.setup (DHT_PIN, DHTesp::DHT22);
  //dht.begin (); // sensor DHT22
  configurarWiFi ();

  // inicializa cliente MQTT
  clienteMQTT.setServer (mqtt_server, 1883);
  clienteMQTT.setCallback (recibirMQTT);
  clienteMQTT.subscribe (topic_sub);

  // pulsadores
  pinMode (PULSADOR_LED1, INPUT_PULLUP);
  pinMode (PULSADOR_LED2, INPUT_PULLUP);
  pinMode (PULSADOR_RESET, INPUT_PULLUP);

  // Leds
  pinMode (LED1, OUTPUT);
  pinMode (LED2, OUTPUT);

  publicarMQTT ();
}

void loop() {
  // si se desconecto intento nuevamente
  if (!clienteMQTT.connected ())
    reconectarMQTT ();

  clienteMQTT.loop ();

  // Leer DHT22 cada 2 segundos
  if (millis() - UltimaLectura >= Intervalo) {
    TempAndHumidity lecturaDHT = dht.getTempAndHumidity ();
    Temperatura = lecturaDHT.temperature;
    Humedad = lecturaDHT.humidity;
    UltimaLectura = millis();
    Serial.print ("Humedad: ");
    Serial.println (Humedad);
    Serial.print ("Temperatura: ");
    Serial.println (Temperatura);
  }

  // Lectura pulsadores
  bool pulsadorLed1 = !digitalRead (PULSADOR_LED1);
  bool pulsadorLed2 = !digitalRead (PULSADOR_LED2);
  bool pulsadorReset = !digitalRead (PULSADOR_RESET);

  // LED1
  if (pulsadorLed1) {
    if (EstadoP1 == false) {
      if (EstadoLed1 == false) {
        // encender
        digitalWrite (LED1, HIGH);
        EstadoLed1 = true;
      }
      else {
        // apagar
        digitalWrite (LED1, LOW);
        EstadoLed1 = false;
      }
      // toggleo variable
      EstadoP1 = true;
      publicarMQTT ();
      delay (300); // antirrebote
    }
  }
  else {
    if (EstadoP1)
      EstadoP1 = false;
  }
  // LED 2
  if (pulsadorLed2) {
    if (EstadoP2 == false) {
      if (EstadoLed2 == false) {
        // enciendo
        digitalWrite (LED2, HIGH);
        EstadoLed2 = true;
      }
      else {
        // apago
        digitalWrite (LED2, LOW);
        EstadoLed2 = false;
      }
      // toggleo variable
      EstadoP2 = true;
      publicarMQTT ();
      delay (300);
    }
  }
  else {
    if (EstadoP2 == true)
      EstadoP2 = false;
  }
  
  // Limpiar pantalla
  if (pulsadorReset) {
    Serial.println ("Reset");
    if (EstadoReset == false) {
      EstadoReset = true;
      Mensaje = "-";
      Pantalla.print ("-");
      publicarMQTT ();
      delay (300);
    }
  }
  else {
    if (EstadoReset == true)
      EstadoReset = false;
  }

  // Mostrar mensaje
  // si es un mensaje corto, lo centra
  if (Mensaje.length() < 10) {
    Pantalla.setTextAlignment (PA_CENTER);
    Pantalla.print (Mensaje.c_str());
  }
  else {
    if (Pantalla.displayAnimate())
      Pantalla.displayText (Mensaje.c_str(), PA_LEFT, 100, 100, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
  }
}