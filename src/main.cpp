// Proyecto y Diseño Electrónico 2025
// Instituto Don Orione
// Prof. Juan Carlos Torres

// Para matriz de leds
#include <MD_MAX72xx.h>
#include <MD_Parola.h>
// Para WiFi
#include <WiFi.h>
#include <WiFiClient.h>
// Libreria MQTT
#include <PubSubClient.h>
// Libreria para soporte Json v7
#include <ArduinoJson.h>

#define HARDWARE_TYPE MD_MAX72XX::PAROLA_HW
#define DATA_PIN    23 // MOSI
#define CLK_PIN     18 // SCK
#define CS_PIN      5  // SS
#define MAX_DEVICES 8
#define MQTT_PORT   1883
// pulsador amarillo (rele 1)
#define P1  34
// pulsador azul (rele 2)
#define P2  35
// pulsador verde (reset)
#define P3 32
#define RELE_1 33
#define RELE_2 25

// variables globales mqtt
const char* mqtt_server = "broker.hivemq.com";
// topico publicacion
const char* topic_pub = "ido_pyde/esp32";
// topico suscripcion (mensaje a mostrar)
const char* topic_sub = "ido_pyde/texto";

#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""
// se define canal para acelerar la conexion
#define WIFI_CHANNEL 6

MD_Parola P = MD_Parola (HARDWARE_TYPE, CS_PIN, MAX_DEVICES); // Conexion SPI

bool Boton1 = false; // amarillo
bool Boton2 = false; // azul
bool Boton3 = false; // verde
String Mensaje = "-";
bool Rele1 = false;
bool Rele2 = false;

WiFiClient clienteWiFi;
PubSubClient clienteMQTT (clienteWiFi);

void publicar ()
{
  // publicacion en topico
  StaticJsonDocument<300> JSONdoc;
  JsonObject JSONencoder = JSONdoc.to<JsonObject>();

  JSONencoder["relay1"] = Rele1;
  JSONencoder["relay2"] = Rele2;
  JSONencoder["led"] = Mensaje;

  char JSONmessageBuffer [100];
  serializeJson (JSONdoc, JSONmessageBuffer);
  clienteMQTT.publish (topic_pub, JSONmessageBuffer);
  Serial.print ("Enviando estado: ");
  Serial.println (JSONmessageBuffer);
  Serial.println ("=======================");
}

void callback (char* topic, byte* payload, unsigned int length)
{
  // recibe un mensaje de un topico suscrito
  Serial.print ("Mensaje recibido en topico: ");
  Serial.println (topic);
  String json_received;
  Serial.print ("JSON Recibido: ");
  for (int i = 0; i < length; i++) {
    json_received += ((char)payload[i]);
    //Serial.print((char)payload[i]);
  }
  Serial.println (json_received);
  // actualizacion estado a la app
  if (json_received == "status")
    publicar();
  else
  {
    //Parse json
    JsonDocument root;
    deserializeJson(root, json_received);
    //sample of json: {"device":"relay1","status":true}
    //sample of json: {"device":"led","text":"Text display"}
    Serial.print("Comando: ");
    String device = root["device"];
    String trigger;
    // Rele 1
    if (device == "relay1") {
      if (root["status"] == true) {
        // enciende
        digitalWrite (RELE_1, HIGH);
        trigger = "ON";
        Rele1 = true;
      }
      else {
        // apaga
        digitalWrite (RELE_1, LOW);
        trigger = "OFF";
        Rele1 = false;
      }
      bool status_relay = root["status"]; // REVISAR
      Serial.println ("Turn " + trigger + " " + device);
    }
    // Rele 2
    if (device == "relay2") {
      if (root["status"] == true) {
        // enciende
        digitalWrite (RELE_2, HIGH);
        trigger = "ON";
        Rele2 = true;
      }
      else {
        // apaga
        digitalWrite (RELE_2, LOW);
        trigger = "OFF";
        Rele2 = false;
      }
      bool status_relay = root["status"];
      Serial.println("Turn " + trigger + " " + device);
    }
    // matriz de leds
    if (device == "led") {
      // mostrar mensaje
      Mensaje = root["text"].as<const char*>();
      Serial.println ("Texto a mostrar: " + Mensaje);
    }
    // envia actualizacion mqtt
    publicar ();
  }
}

void reconectar () 
{
  while (!clienteMQTT.connected())
  {
    Serial.print ("Intentando conexion MQTT con ");
    Serial.print (mqtt_server);
    Serial.print ("... ");
    // random client ID
    String clientId = "IDO-";
    clientId += String (random(0xffff), HEX);
    // if (client.connect(clientId,userName,passWord))
    if (clienteMQTT.connect(clientId.c_str()))
    {
      Serial.print ("Conexion exitosa [");
      Serial.print (clientId.c_str());
      Serial.println ("]");
      // suscripcion a topico
      clienteMQTT.subscribe (topic_sub);
      Serial.print ("Suscrito a topico: ");
      Serial.println (topic_sub);
    } else {
      Serial.print ("[Error =");
      Serial.print (clienteMQTT.state());
      Serial.print ("]");
      Serial.println (" Reintentando en 5 segundos...");
      delay(5000);
    }
  }
}

void setup (void)
{
  Serial.begin (115200);

  WiFi.begin (WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
  Serial.print ("Conectando a WiFi: ");
  Serial.print (WIFI_SSID);
  // Espera conexion Wi-Fi
  while (WiFi.status () != WL_CONNECTED) {
    delay (500);
    Serial.print (".");
  }
  Serial.print (" Conectado!");
  Serial.print ("(IP: ");
  Serial.print (WiFi.localIP());
  Serial.println (")");

  clienteMQTT.setServer (mqtt_server, MQTT_PORT);
  clienteMQTT.setCallback (callback);
  clienteMQTT.subscribe (topic_sub);

  // Pines
  pinMode (P1, INPUT_PULLUP);
  pinMode (P2, INPUT_PULLUP);
  pinMode (P3, INPUT_PULLUP);
  pinMode (RELE_1, OUTPUT);
  pinMode (RELE_2, OUTPUT);

  // inicia con reles apagados
  digitalWrite (RELE_1, LOW);
  digitalWrite (RELE_2, LOW);

  // actualizacion estado MQTT
  publicar ();
}

void loop() {

  if (!clienteMQTT.connected ())
    reconectar ();

  clienteMQTT.loop ();
  
  bool sePulsoP1 = !digitalRead (P1);
  bool sePulsoP2 = !digitalRead (P2);
  bool sePulsoP3 = !digitalRead (P3);

  // Pulsador 1 (amarillo/rele 1)
  if (sePulsoP1) {
    if (Boton1 == false) {
      Boton1 = true; // para reflejar el cambio en la app
      if (Rele1 == false) {
        // activo rele 1
        digitalWrite (RELE_1, HIGH);
        Rele1 = true;
      }
      else {
        // apago rele 1
        digitalWrite (RELE_1, LOW);
        Rele1 = false;
      }
      delay (100); // antirrebote
      publicar ();
    }
  }
  else {
    if (Boton1 == true)
      Boton1 = false;
  }

  // Pulsador 2 (azul/rele 2)
  if (sePulsoP2) {
    if (Boton2 == false) {
      Boton2 = true; // para reflejar el cambio en la app
      if (Rele2 == false) {
        // activo rele 2
        digitalWrite (RELE_2, HIGH);
        Rele2 = true;
      }
      else {
        // apago rele 2
        digitalWrite (RELE_2, LOW);
        Rele2 = false;
      }
      Boton2 = true;
      delay (100); // antirrebote
      publicar ();
    }
  }
  else {
    if (Boton2 == true)
      Boton2 = false;
  }

  // Pulsador 3 (verde/reset)
  if (sePulsoP3) {
    if (Boton3 == false) {
      // limpio pantalla
      Boton3 = true;
      Mensaje = "-";
      P.print ("-");
      delay (100); // antirrebote
      publicar ();
    }
  }
  else {
    if (Boton3 == true)
      Boton3 = false;
  }

  //Show Text in LED Matrix
  if (Mensaje.length () < 10) {
    P.setTextAlignment (PA_CENTER);
    P.print (Mensaje.c_str ());
  }
  else {
    if (P.displayAnimate ())
      P.displayText (Mensaje.c_str (), PA_LEFT, 100, 100, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
  }
}