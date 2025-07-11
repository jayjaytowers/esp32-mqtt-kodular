// By Yaser Ali Husen
// Using ESP8266
// Receive message from mqtt broker to turn on device
// Reference GPIO  https://randomnerdtutorials.com/esp8266-pinout-reference-gpios/

//For Led Matrix
#include <MD_MAX72xx.h>
#include <MD_Parola.h>
//Setup for MQTT and WiFi
#include <WiFi.h>
#include <WiFiClient.h>
//Library for MQTT:
#include <PubSubClient.h>
//Library for Json format using version 7, please update ArduinoJson Library if you still in version 5 or 6
#include <ArduinoJson.h>

#define HARDWARE_TYPE MD_MAX72XX::PAROLA_HW
#define DATA_PIN 23 // MOSI
#define CLK_PIN 18  // SCK
#define CS_PIN 5    // SS
#define MAX_DEVICES 8
#define MQTT_PORT   1883
//const char* mqtt_server = "106.227.140.110"; //without port
const char* mqtt_server = "broker.hivemq.com";

//declare topic for publish message
const char* topic_pub = "esp8266";
//declare topic for subscribe message
const char* topic_sub = "kodular";

#define WIFI_SSID "Wokwi-GUEST"
#define WIFI_PASSWORD ""
// Defining the WiFi channel speeds up the connection:
#define WIFI_CHANNEL 6

//Setup for LED dot Max7219==============================
// Hardware SPI connection
MD_Parola P = MD_Parola (HARDWARE_TYPE, CS_PIN, MAX_DEVICES);
// Arbitrary output pins
// MD_Parola P = MD_Parola(HARDWARE_TYPE, DATA_PIN, CLK_PIN, CS_PIN, MAX_DEVICES);

//for input/output
int BUTTON_PIN1 = 34;    // amarillo
int BUTTON_PIN2 = 35;    // azul
int BUTTON_RESET = 32;   // verde

int relay1 = 33;         // rele 1
int relay2 = 25;         // rele 2

bool status_button1 = false;
bool status_button2 = false;
bool status_reset = false;
String msg = "-";
bool status_relay1 = false;
bool status_relay2 = false;

WiFiClient espClient;
PubSubClient client (espClient);

// Global data
struct sCatalog
{
  textEffect_t  effect;   // text effect to display
  const char *  psz;      // text string nul terminated
  uint16_t      speed;    // speed multiplier of library default
  uint16_t      pause;    // pause multiplier for library default
};

sCatalog catalog[] =
{
  { PA_PRINT, "PRINT", 1, 1 },
  { PA_SCROLL_UP, "SC_U", 5, 1 },
  { PA_SCROLL_DOWN, "SCR_D", 5, 1 },
  { PA_SCROLL_LEFT, "SCR_L", 5, 1 },
  { PA_SCROLL_RIGHT, "SCR_R", 5, 1 },
#if ENA_SPRITE
  { PA_SPRITE, "SPRIT", 5, 1 },
#endif
#if ENA_MISC
  { PA_SLICE, "SLICE", 1, 1 },
  { PA_MESH, "MESH", 20, 1 },
  { PA_FADE, "FADE", 20, 1 },
  { PA_DISSOLVE, "DSLVE", 7, 1 },
  { PA_BLINDS, "BLIND", 7, 1 },
  { PA_RANDOM, "RAND", 3, 1 },
#endif
#if ENA_WIPE
  { PA_WIPE, "WIPE", 5, 1 },
  { PA_WIPE_CURSOR, "WPE_C", 4, 1 },
#endif
#if ENA_SCAN
  { PA_SCAN_HORIZ, "SCNH", 4, 1 },
  { PA_SCAN_HORIZX, "SCNHX", 4, 1 },
  { PA_SCAN_VERT, "SCNV", 3, 1 },
  { PA_SCAN_VERTX, "SCNVX", 3, 1 },
#endif
#if ENA_OPNCLS
  { PA_OPENING, "OPEN", 3, 1 },
  { PA_OPENING_CURSOR, "OPN_C", 4, 1 },
  { PA_CLOSING, "CLOSE", 3, 1 },
  { PA_CLOSING_CURSOR, "CLS_C", 4, 1 },
#endif
#if ENA_SCR_DIA
  { PA_SCROLL_UP_LEFT, "SCR_UL", 7, 1 },
  { PA_SCROLL_UP_RIGHT, "SCR_UR", 7, 1 },
  { PA_SCROLL_DOWN_LEFT, "SCR_DL", 7, 1 },
  { PA_SCROLL_DOWN_RIGHT, "SCR_DR", 7, 1 },
#endif
#if ENA_GROW
  { PA_GROW_UP, "GRW_U", 7, 1 },
  { PA_GROW_DOWN, "GRW_D", 7, 1 },
#endif
};

static textPosition_t justy[] = { PA_LEFT, PA_CENTER, PA_RIGHT };

// Sprite Definitions
const uint8_t F_PMAN1 = 6;
const uint8_t W_PMAN1 = 8;
static const uint8_t PROGMEM pacman1[F_PMAN1 * W_PMAN1] =  // gobbling pacman animation
{
  0x00, 0x81, 0xc3, 0xe7, 0xff, 0x7e, 0x7e, 0x3c,
  0x00, 0x42, 0xe7, 0xe7, 0xff, 0xff, 0x7e, 0x3c,
  0x24, 0x66, 0xe7, 0xff, 0xff, 0xff, 0x7e, 0x3c,
  0x3c, 0x7e, 0xff, 0xff, 0xff, 0xff, 0x7e, 0x3c,
  0x24, 0x66, 0xe7, 0xff, 0xff, 0xff, 0x7e, 0x3c,
  0x00, 0x42, 0xe7, 0xe7, 0xff, 0xff, 0x7e, 0x3c,
};

const uint8_t F_PMAN2 = 6;
const uint8_t W_PMAN2 = 18;
static const uint8_t PROGMEM pacman2[F_PMAN2 * W_PMAN2] =  // ghost pursued by a pacman
{
  0x00, 0x81, 0xc3, 0xe7, 0xff, 0x7e, 0x7e, 0x3c, 0x00, 0x00, 0x00, 0xfe, 0x7b, 0xf3, 0x7f, 0xfb, 0x73, 0xfe,
  0x00, 0x42, 0xe7, 0xe7, 0xff, 0xff, 0x7e, 0x3c, 0x00, 0x00, 0x00, 0xfe, 0x7b, 0xf3, 0x7f, 0xfb, 0x73, 0xfe,
  0x24, 0x66, 0xe7, 0xff, 0xff, 0xff, 0x7e, 0x3c, 0x00, 0x00, 0x00, 0xfe, 0x7b, 0xf3, 0x7f, 0xfb, 0x73, 0xfe,
  0x3c, 0x7e, 0xff, 0xff, 0xff, 0xff, 0x7e, 0x3c, 0x00, 0x00, 0x00, 0xfe, 0x73, 0xfb, 0x7f, 0xf3, 0x7b, 0xfe,
  0x24, 0x66, 0xe7, 0xff, 0xff, 0xff, 0x7e, 0x3c, 0x00, 0x00, 0x00, 0xfe, 0x73, 0xfb, 0x7f, 0xf3, 0x7b, 0xfe,
  0x00, 0x42, 0xe7, 0xe7, 0xff, 0xff, 0x7e, 0x3c, 0x00, 0x00, 0x00, 0xfe, 0x73, 0xfb, 0x7f, 0xf3, 0x7b, 0xfe,
};

void setup (void)
{
  Serial.begin (115200);

  WiFi.begin (WIFI_SSID, WIFI_PASSWORD, WIFI_CHANNEL);
<<<<<<< HEAD
  Serial.print ("Conectando a WiFi: ");
  Serial.print (WIFI_SSID);
  // Wait for connection
  while (WiFi.status () != WL_CONNECTED) {
    delay (500);
    Serial.print (".");
  }
  Serial.println ("");
  Serial.print (" Conectado!");
  Serial.print ("(IP: ");
  Serial.print (WiFi.localIP());
  Serial.println (")");

  client.setServer (mqtt_server, MQTT_PORT);
  client.setCallback (callback);
  //subscribe topic
  client.subscribe (topic_sub);

  //setup pin input
  pinMode (BUTTON_PIN1, INPUT_PULLUP);
  pinMode (BUTTON_PIN2, INPUT_PULLUP);
  pinMode (BUTTON_RESET, INPUT_PULLUP);

  //setup pin output
  pinMode (relay1, OUTPUT);
  pinMode (relay2, OUTPUT);

  //Reset lamp, turn off all, because I use relay with gnd as trigger so I set LOW to turn on
  digitalWrite (relay1, HIGH);
  digitalWrite (relay2, HIGH);
=======
  Serial.print ("Connecting to WiFi ");
  Serial.print (WIFI_SSID);
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay (500);
    Serial.print (".");
  }
  Serial.println (" Connected!");
  Serial.print ("IP address: ");
  Serial.println (WiFi.localIP());

  client.setServer(mqtt_server, MQTT_PORT);
  client.setCallback(callback);
  //subscribe topic
  client.subscribe(topic_sub);

  //setup pin input
  pinMode(BUTTON_PIN1, INPUT_PULLUP);
  pinMode(BUTTON_PIN2, INPUT_PULLUP);
  pinMode(BUTTON_RESET, INPUT_PULLUP);

  //setup pin output
  pinMode(relay1, OUTPUT);
  pinMode(relay2, OUTPUT);

  //Reset lamp, turn off all, because I use relay with gnd as trigger so I set LOW to turn on
  digitalWrite(relay1, HIGH);
  digitalWrite(relay2, HIGH);
>>>>>>> 8090bb36b435367ceaac5c07c614ab7c03a63c3f

  P.begin ();
#if ENA_SPRITE
  P.setSpriteData(pacman1, W_PMAN1, F_PMAN1, pacman2, W_PMAN2, F_PMAN2);
#endif

<<<<<<< HEAD
  for (uint8_t i=0; i<ARRAY_SIZE(catalog); i++) {
    catalog[i].speed *= P.getSpeed();
    catalog[i].pause *= 500;
  }
  send_stat ();
=======
  for (uint8_t i=0; i<ARRAY_SIZE(catalog); i++)
  {
    catalog[i].speed *= P.getSpeed();
    catalog[i].pause *= 500;
  }
  send_stat();
  
>>>>>>> 8090bb36b435367ceaac5c07c614ab7c03a63c3f
}

void loop(void)
{
  if (!client.connected ())
    reconnect ();

  client.loop ();

<<<<<<< HEAD
  if (P.displayAnimate ()) // animates and returns true when an animation is completed
=======
  if (P.displayAnimate()) // animates and returns true when an animation is completed
>>>>>>> 8090bb36b435367ceaac5c07c614ab7c03a63c3f
  {
    uint8_t eIn = random(ARRAY_SIZE(catalog));  // text effect IN index
    uint8_t eOut = random(ARRAY_SIZE(catalog)); // text effect OUT index
    uint8_t j = random(ARRAY_SIZE(justy));      // text justification index

    // set up all the new animation parameters manually
    P.setTextBuffer(catalog[eIn].psz);
    P.setTextAlignment(justy[j]);
    P.setSpeedInOut(catalog[eIn].speed, catalog[eOut].speed);
    P.setPause(catalog[eIn].pause);
    P.setTextEffect(catalog[eIn].effect, catalog[eOut].effect);
    P.displayReset();
  }
}

<<<<<<< HEAD
void send_stat ()
=======
void send_stat()
>>>>>>> 8090bb36b435367ceaac5c07c614ab7c03a63c3f
{
  //check output status--------------------------------------------
  //This function will send the current condition to kodular app
  StaticJsonDocument<300> JSONdoc;
  JsonObject JSONencoder = JSONdoc.to<JsonObject>();

  JSONencoder["relay1"] = status_relay1;
  JSONencoder["relay2"] = status_relay2;
  JSONencoder["led"] = msg;

<<<<<<< HEAD
  char JSONmessageBuffer [100];
  serializeJson (JSONdoc, JSONmessageBuffer);
  client.publish (topic_pub, JSONmessageBuffer);
  Serial.print ("Enviando estado: ");
  Serial.println (JSONmessageBuffer);
  Serial.println ("=======================");
=======
  char JSONmessageBuffer[100];
  serializeJson(JSONdoc, JSONmessageBuffer);
  client.publish(topic_pub, JSONmessageBuffer);
  Serial.println("Send Status:");
  Serial.println(JSONmessageBuffer);
  Serial.println("=======================");
>>>>>>> 8090bb36b435367ceaac5c07c614ab7c03a63c3f
}

void callback(char* topic, byte* payload, unsigned int length)
{
  //Receiving message as subscriber
<<<<<<< HEAD
  Serial.print("Mensaje recibido en topico: ");
  Serial.println(topic);
  String json_received;
  Serial.print("JSON Recibido: ");
=======
  Serial.print("Message arrived in topic: ");
  Serial.println(topic);
  String json_received;
  Serial.print("JSON Received:");
>>>>>>> 8090bb36b435367ceaac5c07c614ab7c03a63c3f
  for (int i = 0; i < length; i++) {
    json_received += ((char)payload[i]);
    //Serial.print((char)payload[i]);
  }
  Serial.println(json_received);
  //if receive ask status from kodular, send current status of relay and led
  if (json_received == "status")
  {
    send_stat();
  }
  else
  {
    //Parse json
    JsonDocument root;
    deserializeJson(root, json_received);

    //get json parsed value
    //sample of json: {"device":"relay1","status":true}
    //sample of json: {"device":"led","text":"Text display"}
<<<<<<< HEAD
    Serial.print("Comando: ");
=======
    Serial.print("Command: ");
>>>>>>> 8090bb36b435367ceaac5c07c614ab7c03a63c3f
    String device = root["device"];
    String trigger;
    //Trigger device
    //relay1***************************
    if (device == "relay1")
    {
      if (root["status"] == true)
      {
        //turn ON
        digitalWrite(relay1, LOW);
        trigger = "ON";
        status_relay1 = true;
      }
      else
      {
        //turn OFF
        digitalWrite(relay1, HIGH);
        trigger = "OFF";
        status_relay1 = false;
      }
      bool status_relay = root["status"];
      Serial.println("Turn " + trigger + " " + device);
    }
    //relay2***************************
    if (device == "relay2")
    {
      if (root["status"] == true)
      {
        //turn ON
        digitalWrite(relay2, LOW);
        trigger = "ON";
        status_relay2 = true;
      }
      else
      {
        digitalWrite(relay2, HIGH);
        trigger = "OFF";
        status_relay2 = false;
      }
      bool status_relay = root["status"];
      Serial.println("Turn " + trigger + " " + device);
    }
    //LED***************************
    if (device == "led")
    {
      //Show text in LED matrix
      msg = root["text"].as<const char*>();
<<<<<<< HEAD
      Serial.println("Texto a mostrar: " + msg);
=======
      Serial.println("Display to Dot Matrix: " + msg);
>>>>>>> 8090bb36b435367ceaac5c07c614ab7c03a63c3f
    }
    //Publish current status
    send_stat();
  }
}

void reconnect() {
  // Loop until we're reconnected
  while (!client.connected())
  {
<<<<<<< HEAD
    Serial.print ("Intentando conexion MQTT con ");
    Serial.print (mqtt_server);
    Serial.print ("...");
    // Create a random client ID
    String clientId = "JCT-";
=======
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ESP8266Client-";
>>>>>>> 8090bb36b435367ceaac5c07c614ab7c03a63c3f
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    //if you MQTT broker has clientID,username and password
    //please change following line to    if (client.connect(clientId,userName,passWord))
    if (client.connect(clientId.c_str()))
    {
<<<<<<< HEAD
      Serial.print ("Conexion exitosa [");
=======
      Serial.print ("connected [");
>>>>>>> 8090bb36b435367ceaac5c07c614ab7c03a63c3f
      Serial.print (clientId.c_str());
      Serial.println ("]");
      //once connected to MQTT broker, subscribe command if any
      client.subscribe(topic_sub);
    } else {
<<<<<<< HEAD
      Serial.print ("[Error =");
      Serial.print (client.state());
      Serial.print ("]");
      Serial.println (" Reintentando en 5 segundos...");
=======
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
>>>>>>> 8090bb36b435367ceaac5c07c614ab7c03a63c3f
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

/*
void loop() {

  //check button
  int buttonValue1 = digitalRead(BUTTON_PIN1);
  int buttonValue2 = digitalRead(BUTTON_PIN2);
  int buttonReset = digitalRead(BUTTON_RESET);

  //button1-----------------------------
  if (buttonValue1 == LOW )
  {
    if (status_button1 == false)
    {
      if (status_relay1 == false) {
        //Turn on relay
        digitalWrite(relay1, LOW);
        status_relay1 = true;
      }
      else {
        //Turn off relay
        digitalWrite(relay1, HIGH);
        status_relay1 = false;
      }
      status_button1 = true;
      delay(100);
      send_stat();
    }
  }
  else if (buttonValue1 == HIGH)
  {
    if (status_button1 == true)
    {
      status_button1 = false;
    }
  }
  //button1-----------------------------
  //button2-----------------------------
  if (buttonValue2 == LOW )
  {
    if (status_button2 == false)
    {
      if (status_relay2 == false) {
        //Turn on relay
        digitalWrite(relay2, LOW);
        status_relay2 = true;
      }
      else {
        //Turn off relay
        digitalWrite(relay2, HIGH);
        status_relay2 = false;
      }
      status_button2 = true;
      delay(100);
      send_stat();
    }
  }
  else if (buttonValue2 == HIGH)
  {
    if (status_button2 == true)
    {
      status_button2 = false;
    }
  }
  //button2-----------------------------
  //buttonreset-----------------------------
  if (buttonReset == LOW )
  {
    if (status_reset == false)
    {
      //Turn off LED
      status_reset = true;
      msg = "-";
      P.print("-");
      delay(100);
      send_stat();
    }
  }
  else if (buttonReset == HIGH)
  {
    if (status_reset == true)
    {
      status_reset = false;
    }
  }
  //buttonreset-----------------------------

  //Show Text in LED Matrix
  if (msg.length() < 10) {
    P.setTextAlignment(PA_CENTER);
    P.print(msg.c_str());
  }
  else {
    if (P.displayAnimate())
      P.displayText(msg.c_str(), PA_LEFT, 100, 100, PA_SCROLL_LEFT, PA_SCROLL_LEFT);
  }

}


*/