#include <Arduino.h>
#include "WiFi.h"
#include "SPIFFS.h"
#include "ESPAsyncWebServer.h"
 
const char* ssid = "MrFlexi";
const char* password =  "Linde-123";
 
AsyncWebServer server(80);
 
void setup(){
  Serial.begin(115200);
 
  if(!SPIFFS.begin()){
     Serial.println("An Error has occurred while mounting SPIFFS");
     return;
  }
 
  WiFi.begin(ssid, password);
 
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }
 
  Serial.println(WiFi.localIP());
 
  server.on("/index", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("Index requested");
    request->send(SPIFFS, "/index.html", "text/html");
  });
 
  server.on("/src/bootstrap.bundle.min.js", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("bootstrap requested");
    request->send(SPIFFS, "/src/bootstrap.bundle.min.js", "text/javascript");
  });
 
  server.on("/src/jquery-3.3.1.min.js", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("jquery requested");
    request->send(SPIFFS, "/src/jquery-3.4.1.min.js", "text/javascript");
  });
 
  server.on("/src/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest *request){
    Serial.println("css requested");
    request->send(SPIFFS, "/src/bootstrap.min.css", "text/css");
  });
 
  server.begin();
  server.serveStatic("/", SPIFFS, "/");
}
 
void loop(){}

