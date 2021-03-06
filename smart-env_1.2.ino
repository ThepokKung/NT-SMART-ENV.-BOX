//include libary
#include <Wire.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include  <LiquidCrystal_I2C.h>
LiquidCrystal_I2C  lcd(0x27, 20, 4);
#include <PMsensor.h>
#include "DHT.h"
#include <ArduinoJson.h>

//define
#define DHTPIN 27
#define DHTTYPE DHT11
char* ssid = "RSRC2"; //Wifi Name
char* password = "043234794"; //Wifi Password
////////////////////////////////////////////////////////////////////////////////////
//MQTT connected
//Real Server
const char* mqttServer = "180.180.216.61";
const int mqttPort = 1883;
const char* mqttUser = "juub";
const char* mqttPassword = "t0t12345";

char mqtt_name_id[] = "";

///////////// //////////////////////////////////////////////////////////////////////
//Vallue
PMsensor PM;
int  samplingTime  =  280;
int  deltaTime  =  40;
int  sleepTime  =  9680;
float  voMeasured  =  0;
float calcVoltage = 0;
float  dustDensity  =  0;
float  temp = 0;
float  humi = 0;
//int LED_BUILTIN = 2;
long now = millis();
unsigned long previousMillis = 0;
unsigned long interval = 30000;

WiFiClient client;
PubSubClient mqtt(client);
DHT dht(DHTPIN, DHTTYPE);

//Json
////////////////////////////////////////////////////
StaticJsonDocument<256> doc;
char buffer[256];
////////////////////////////////////////////////////
void setup() {
  // initialize the LCD
  lcd.begin();
  // Turn on the blacklight and print a message.
  lcd.backlight();
  lcd.clear();
  //  Print  a  message  to  the  LCD.
  lcd.clear();
  lcd.setCursor (9, 1);
  lcd.print("NT");
  lcd.setCursor (3, 2);
  lcd.print("SMART ENV. BOX");
  delay(3000);

  dht.begin();
  Serial.begin(115200);
  PM.init(16, 36);

  //Login all
  setupwifi();
  mqtt.setServer(mqttServer, mqttPort); //Login MQTT
  mqtt.setCallback(callback);
}

void setuplcd() {
  //  Print  a  message  to  the  LCD.
  lcd.backlight();
  lcd.setCursor(3, 0);
  lcd.print("TEMP:");
  lcd.setCursor(15, 0);
  lcd.print("C ");

  lcd.setCursor(3, 1);
  lcd.print("HUMI:");
  lcd.setCursor(15, 1);
  lcd.print("%");

  lcd.setCursor(0, 2);
  lcd.print("PM2.5:");
  lcd.setCursor(15, 2);
  lcd.print("ug/m3");
}

void setupwifi() {
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  WiFi.begin(ssid, password);
  lcd.clear();
  lcd.setCursor(4, 0);
  lcd.print("Wifi Status");
  lcd.setCursor(2, 1);
  lcd.print("Connecting to");
  lcd.setCursor(0, 2);
  lcd.print(ssid);

  while (WiFi.status() != WL_CONNECTED) {
    delay(1500);
    Serial.print(".");
  }

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  lcd.clear();
  lcd.setCursor(6, 2);
  lcd.print("Succeed!");
  delay(2000);
  lcd.clear();
}

void callback (char* topic, byte* payload, unsigned int length) {
  deserializeJson(doc, payload, length);
  size_t n = serializeJson(doc, buffer);
  ///////////////////////////////////////////////////////////////////////////////////////////////
  if (mqtt.connect("ntnode/sensors/", mqttUser, mqttPassword)) {
    if (mqtt.publish("ntnode/sensors/", buffer, n) == true) {
      Serial.println("publish Valve status success");

    } else {
      Serial.println("publish Fail");
    }
  } else {
    Serial.println("Connect Fail MQTT");
  }
}

void reconnect_mqtt() {
  while (!mqtt.connected()) {
    unsigned long currentMillis = millis();
    // if WiFi is down, try reconnecting every CHECK_WIFI_TIME seconds
    if ((WiFi.status() != WL_CONNECTED) && (currentMillis - previousMillis >= interval)) {
      Serial.print(millis());
      Serial.println("Reconnecting to WiFi...");
      WiFi.disconnect();
      WiFi.reconnect();
      previousMillis = currentMillis;
    }
    Serial.print("Attempting MQTT connection...");
    reconnect_lcd_text();
    //WiFi.reconnect();
    if (mqtt.connect(mqtt_name_id, mqttUser, mqttPassword)) {
      Serial.println("connected");
      lcd.setCursor(6, 3);
      lcd.print("Succeed");
      delay(500);
      lcd.clear();
      setuplcd();
      // Subscribe or resubscribe to a topic
      // You can subscribe to more topics (to control more LEDs in this example)
    } else {
      Serial.print("failed, rc=");
      Serial.print(mqtt.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void reconnect_lcd_text() {
  lcd.clear();
  lcd.setCursor(3, 1);
  lcd.print("Server Down");
  lcd.setCursor(1, 2);
  lcd.print("Reconnect to MQTT");
}

void loop() {
  setuplcd();
  // Reconnect MQTT
  if (!mqtt.connected()) {
    Serial.println("---Reconnect MQTT ---");
    reconnect_mqtt();
  }
  mqtt.loop();
  /////////////////////////////////////////////////////////////////////////////////////

  float h = dht.readHumidity();
  // Read temperature as Celsius (the default)
  float t = dht.readTemperature();
  // Read temperature as Fahrenheit (isFahrenheit = true)
  float f = dht.readTemperature(true);
  // Check if any reads failed and exit early (to try again)
  if (isnan(h) || isnan(t) || isnan(f)) {
    Serial.println(F("Failed to read from DHT sensor!"));
    return;
  }
  // Compute heat index in Fahrenheit (the default)
  float hif = dht.computeHeatIndex(f, h);
  // Compute heat index in Celsius (isFahreheit = false)
  float hic = dht.computeHeatIndex(t, h, false);

  float filter_Data = PM.read(0.1);
  float noFilter_Data = PM.read();

  /////////////////////////////////////////////////////////////////////////////////////

  temp = t;
  humi = h;

  /////////////////////////////////////////////////////////////////////////////////////

  calcVoltage  =  noFilter_Data  *  (5.0 /  1024.0);
  dustDensity  =  17.1  *  calcVoltage  -  0.1;
  if (dustDensity >= 230) dustDensity = 231;

  lcd.setCursor(9, 0);
  lcd.print(t);

  lcd.setCursor(9, 1);
  lcd.print(h);

  lcd.setCursor(8, 2);

  if (dustDensity < 0) {
    lcd.print("0");
  } else if (dustDensity > 230) {
    lcd.print("> 230");
  } else {
    lcd.print(dustDensity);
  }
  if (dustDensity >= 201) { //201++
    lcd.setCursor(5, 3);
    lcd.print("---Danger---");
    //Serial.println("---Danger---");
  } else if (dustDensity >= 151) { //151-200
    lcd.setCursor(5, 3);
    lcd.print("--VeryHigh--");
    //Serial.println("--VeryHigh--");
  } else if (dustDensity >= 101) { //101-150
    lcd.setCursor(5, 3);
    lcd.print("----High----");
    //Serial.println("----High----");
  } else if (dustDensity >= 51) {  //51-100
    lcd.setCursor(5, 3);
    lcd.print("---Medium---");
    //Serial.println("---Medium---");
  } else if (dustDensity >= 26) {  // 26-50
    lcd.setCursor(5, 3);
    lcd.print("----Good----");
    //Serial.println("----Good----");
  } else {                         //0-25
    lcd.setCursor(5, 3);
    lcd.print("--VeryGood--");
    //Serial.println("--VeryGood--");
  }

  //////////////////////////////////////////////////////////////////////////////////////
  Serial.println("Device id : SMART ENV. BOX");
  Serial.print("Humidity : ");
  Serial.println(h);
  Serial.print("Temperature : ");
  Serial.println(t);
  Serial.print("DustDensity :");
  Serial.println(dustDensity);
  //////////////////////////////////////////////////////////////////////////////////////

  /////////////////////////////////////////////////////////////////////////////////////

  if (dustDensity < 0) { //201++
    doc["deviceid"] = "SMART ENV. BOX";
    doc["temp"] = t;
    doc["humi"] = h;
    doc["dust"] = 0;
  } else {
    doc["deviceid"] = "SMART ENV. BOX";
    doc["temp"] = t;
    doc["humi"] = h;
    doc["dust"] = dustDensity;
  }

  /////////////////////////////////////////////////////////////////////////////////////
  size_t n = serializeJson(doc, buffer);

  if (mqtt.connect(mqtt_name_id, mqttUser, mqttPassword)) {
    Serial.println("\nConnected MQTT: ");
    if (mqtt.publish("ntnode/sensors/", buffer , n) == true) {
      Serial.println("publish success");
    } else {
      Serial.println("publish Fail");
    }
  } else {
    Serial.println("Connect Fail MQTT");
  }
  Serial.println("=============");
  serializeJsonPretty(doc, Serial);
  Serial.println("=============");
  /////////////////////////////////////////////////////////////////////////////////////
  delay(5000);
}
