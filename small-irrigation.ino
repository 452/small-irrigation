#include <ESP8266WiFi.h>
#include <ThingerWifi.h>
#include <ESP8266mDNS.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>
#include <ESP8266WebServer.h>
#define ET_DRIFT 3
#include "everytime.h"

#define MOSTURE_SENSOR_IN 0
#define IRRIGATION_WATER_PUMP_POUT 5

const char* ssid = "";
const char* password = "";
const char* host = "irrigation-system";

int ledPin = 13;
//#define N_DIMMERS 3/
//int dimmer_pin[] = {14, 5, 15/};
volatile int sensorValue;
//ADC_MODE(ADC_VCC);
MDNSResponder mdns;
ESP8266WebServer server(80);
ThingerWifi thing("", "", "%L9M!");

unsigned long previousMillis = 0;
const unsigned long interval = 100; //12UL*60UL*60UL*1000UL;

void handleRoot() {
  server.send(200, "text/plain", "<irrigation><soilHumidity>" + String(map(analogRead(MOSTURE_SENSOR_IN), 955, 430, 0, 100), DEC) + "</soilHumidity>" + "\n<rssi>" + String(WiFi.RSSI(), DEC) + "</rssi>" + "\n<vcc>" + String(ESP.getVcc() / 1024.00f, DEC) + "</vcc></irrigation>");
}

void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

void setup() {

  Serial.begin(115200);

  /* switch on led */
  pinMode(ledPin, OUTPUT);
  pinMode(IRRIGATION_WATER_PUMP_POUT, OUTPUT);
  digitalWrite(ledPin, LOW);
  digitalWrite(IRRIGATION_WATER_PUMP_POUT, LOW);
  Serial.println("Booting");
  WiFi.mode(WIFI_STA);
  WiFi.hostname(host);
  WiFi.begin(ssid, password);

  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    WiFi.begin(ssid, password);
    Serial.println("Retrying connection...");
  }
  /* switch off led */
  digitalWrite(ledPin, HIGH);

  /* configure dimmers, and OTA server events */
  analogWriteRange(1000);
  analogWrite(ledPin, 990);

  ///

  if (mdns.begin(host, WiFi.localIP())) {
    Serial.println("MDNS responder started");
  }

  server.on("/", handleRoot);

  server.on("/inline", []() {
    server.send(200, "text/plain", "this works as well");
  });

  server.onNotFound(handleNotFound);

  server.begin();
  ArduinoOTA.setHostname(host);
  ArduinoOTA.onStart([]() { // switch off all the PWMs during upgrade
    //    for (int i = 0; i < N_DIMMERS; i++)
    //      analogWrite(dimmer_pin[i], 0);
    analogWrite(ledPin, 0);
  });

  ArduinoOTA.onEnd([]() { // do a fancy thing with our board led at end
    for (int i = 0; i < 30; i++)
    {
      analogWrite(ledPin, (i * 100) % 1001);
      delay(50);
    }
  });

  ArduinoOTA.onError([](ota_error_t error) {
    ESP.restart();
  });

  /* setup the OTA server */
  ArduinoOTA.begin();
  thing["irrigation"] >> [](pson & out) {
    sensorValue = map(analogRead(MOSTURE_SENSOR_IN), 955, 430, 0, 100);
    out["soilHumidity"] = sensorValue;
    out["rssi"] = WiFi.RSSI();
  };
  Serial.println("IrrigationSystem Ready");

}

/*
  955 need water
  700 small piece of water
  460 normal maybe
  430 all in water
*/
void loop() {
  ArduinoOTA.handle();
  server.handleClient();
  thing.handle();
  ///
//  digitalWrite(IRRIGATION_WATER_/PUMP_POUT, HIGH);
  ///
  //  Serial.println(sensorValue);
  //  Serial.println("Clean:");
  //  Serial.println(sensorValue);
}
