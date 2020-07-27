#include <ESP8266WiFi.h>
#include <ESP8266mDNS.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <WiFiUdp.h>
#include <ArduinoOTA.h>

#define DOOR_TRIGGER_PIN D1 //I'm using D1 cause it is LOW at boot, I don't want to trigger the door each time the ESP is rebooted.

#define WIFI_SSID "Burton"
#define WIFI_PSK  "Takeachance01"

#define HTTP_PORT 80
#define HTTP_USER "mo"
#define HTTP_PASSWORD "a7jhs6"

const char* ssid = WIFI_SSID;
const char* password = WIFI_PSK;
//LED_BUILTIN
ESP8266WebServer server(HTTP_PORT);


int writeLog(String text) {
  if (Serial) {
    Serial.print(millis());
    Serial.print("    ");
    Serial.println(text);
  }
}

void handleRoot() {
  server.send(415, "text/plain", "Hello, this is the GarageDoorOpener, you should use me as an RESTApi.");
}

void handleTrigger() {
  if (!server.authenticate(HTTP_USER, HTTP_PASSWORD))
    return server.requestAuthentication();

  digitalWrite(DOOR_TRIGGER_PIN, 1);
  delay(1000);
  digitalWrite(DOOR_TRIGGER_PIN, 0);
  delay(500);
  server.send(201);
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

  pinMode(DOOR_TRIGGER_PIN, OUTPUT);
  digitalWrite(DOOR_TRIGGER_PIN, LOW);
  
  Serial.begin(115200);
  Serial.println("Booting");
  
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.waitForConnectResult() != WL_CONNECTED) {
    Serial.println("Connection Failed! Rebooting...");
    delay(5000);
    ESP.restart();
  }

  WiFi.setSleepMode(WIFI_LIGHT_SLEEP);

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
 
  Serial.println("");
  Serial.println("");

  ArduinoOTA.setHostname("garagedoor-opener");
  ArduinoOTA.setPassword("admin2");

  ArduinoOTA.onStart([]() {
    String type;
    if (ArduinoOTA.getCommand() == U_FLASH) {
      type = "sketch";
    } else { // U_FS
      type = "filesystem";
    }

    // NOTE: if updating FS this would be the place to unmount FS using FS.end()
    Serial.println("Start updating " + type);
  });
  ArduinoOTA.onEnd([]() {
    Serial.println("\nEnd");
  });
  ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {
    Serial.printf("Progress: %u%%\r", (progress / (total / 100)));
  });
  ArduinoOTA.onError([](ota_error_t error) {
    Serial.printf("Error[%u]: ", error);
    if (error == OTA_AUTH_ERROR) {
      Serial.println("Auth Failed");
    } else if (error == OTA_BEGIN_ERROR) {
      Serial.println("Begin Failed");
    } else if (error == OTA_CONNECT_ERROR) {
      Serial.println("Connect Failed");
    } else if (error == OTA_RECEIVE_ERROR) {
      Serial.println("Receive Failed");
    } else if (error == OTA_END_ERROR) {
      Serial.println("End Failed");
    }
  });
  ArduinoOTA.begin();
  
  writeLog("OTA is Ready");
  writeLog("Now loading main program...");

  if (MDNS.begin("garagedoor-opener"))
    Serial.println("MDNS responder started");

  server.on("/", handleRoot);
  server.on("/trigger", handleTrigger);
  server.onNotFound(handleNotFound);
  server.begin();
  
}

void loop() {
  
  ArduinoOTA.handle();
  server.handleClient();
  MDNS.update();

}
