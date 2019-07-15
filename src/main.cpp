#include <Arduino.h>
#include "WiFi.h"
#include "SPIFFS.h"
#include "ESPAsyncWebServer.h"
#include <ArduinoJson.h>

const char *ssid = "MrFlexi";
const char *password = "Linde-123";
String JsonStr;

StaticJsonDocument<200> doc;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  switch (type)
  {

  case WS_EVT_CONNECT:
  {
    Serial.println("Websocket client connection received");
    
    doc["sensor"] = "gps";
    doc["time"] = 1351824120;
    JsonArray data = doc.createNestedArray("data");
    data.add(48.756080);
    data.add(2.302038);

    serializeJson(doc, JsonStr);
    client->text(JsonStr);
  }
  break;

  case WS_EVT_DISCONNECT:
  {
    Serial.println("Client disconnected");
  }
  break;

  case WS_EVT_DATA:
  {
    Serial.println("Text received");
  }
  break;
  }
}

void setup()
{
  Serial.begin(9600);

  if (!SPIFFS.begin())
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
    Serial.println("Connecting to WiFi..");
  }

  Serial.println(WiFi.localIP());

  server.on("/index", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Index requested");
    request->send(SPIFFS, "/index.html", "text/html");
  });

  server.on("/src/bootstrap.bundle.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("bootstrap requested");
    request->send(SPIFFS, "/src/bootstrap.bundle.min.js", "text/javascript");
  });

  server.on("/src/jquery-3.3.1.min.js", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("jquery requested");
    request->send(SPIFFS, "/src/jquery-3.4.1.min.js", "text/javascript");
  });

  server.on("/src/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("css requested");
    request->send(SPIFFS, "/src/bootstrap.min.css", "text/css");
  });

  // Websocket
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  server.begin();
  server.serveStatic("/", SPIFFS, "/");
}

void loop() {}
