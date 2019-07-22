#include <Arduino.h>
#include "WiFi.h"
#include "SPIFFS.h"
#include "ESPAsyncWebServer.h"
#include <ArduinoJson.h>
#include "esp_system.h"

TaskHandle_t task_alive_msg;
TaskHandle_t task_cpu_temp;
TaskHandle_t task_broadcast_message;

const char *ssid = "MrFlexi";
const char *password = "Linde-123";
const float alive_msg_intervall = 20; // 60 seconds
int roundtrips = 0;

StaticJsonDocument<500> doc;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

extern "C" {
uint8_t temprature_sens_read();
}

typedef struct {
  int roundtrips;
  float temperatur;
  int32_t latitude;
  int32_t longitude;
} message_buffer_t;

message_buffer_t gs_message_buffer;

void t_cpu_temp(void *parameter)
{

  for (;;)
  {
  //get internal temp of ESP32
  uint8_t temp_farenheit= temprature_sens_read();
  //convert farenheit to celcius
  gs_message_buffer.temperatur = ( temp_farenheit - 32 ) / 1.8;
  
  Serial.print("internal temp [°C]: ");
  Serial.println(gs_message_buffer.temperatur);
    delay(60000);
  } 
}


String message_buffer_to_jsonstr( void )
{
    String JsonStr;
    doc.clear();
    
    doc["roundtrips"] = String(gs_message_buffer.roundtrips );
    doc["sensor"] = "gps";
    doc["time"] = "10:05";
    doc["text"] = "Hallo Welt";
    doc["text_time"] = "SA 8:22:01";

    doc["temperatur"] = String(gs_message_buffer.temperatur );


    // Add the "feeds" array
    JsonArray feeds = doc.createNestedArray("text_table");

    for (int i = 0; i < 1; i++)
    {
      JsonObject msg = feeds.createNestedObject();
      msg["Title"] = "Hallo Welt";
      msg["Description"] = "400m Schwimmen in 4 Minuten";
      msg["Date"] = "13.10.1972";
      msg["Priority"] = "High";
      feeds.add(msg);
    }
    serializeJson(doc, JsonStr);
    serializeJsonPretty(doc, Serial);
    return JsonStr;
}

void t_alive_msg(void *parameter)
{
  // Task bound to core 0, Prio 0 =  very low
  String JsonStr;
  String text = "Serial output \r\n booting.... \r\n nodejs started";

  for (;;)
  {
    JsonStr = "";
    roundtrips++;
    Serial.println("alive ticker");
    Serial.print("running on core: ");
    Serial.println(xPortGetCoreID());

    gs_message_buffer.roundtrips = roundtrips;

    
    serializeJson(doc, JsonStr);
    ws.textAll(JsonStr);
    serializeJsonPretty(doc, Serial);
    delay(20000);
  }
}

void t_broadcast_message(void *parameter)
{
  // Task bound to core 0, Prio 0 =  very low
  String JsonStr;

  for (;;)
  {
    JsonStr = message_buffer_to_jsonstr();
    ws.textAll(JsonStr);
    delay(10000);
  }
}

void create_Tasks()
{

  xTaskCreate(
      t_broadcast_message,      /* Task function. */
      "Broadcast Message",   /* String with name of task. */
      10000,            /* Stack size in bytes. */
      NULL,             /* Parameter passed as input of the task */
      10,                /* Priority of the task. */
      &task_broadcast_message); /* Task handle. */

  xTaskCreate(
      t_alive_msg,      /* Task function. */
      "AliveMessage",   /* String with name of task. */
      10000,            /* Stack size in bytes. */
      NULL,             /* Parameter passed as input of the task */
      0,                /* Priority of the task. */
      &task_alive_msg); /* Task handle. */

  xTaskCreate(
      t_cpu_temp,         /* Task function. */
      "CPUTemp",       /* String with name of task. */
      10000,           /* Stack size in bytes. */
      NULL,            /* Parameter passed as input of the task */
      0,               /* Priority of the task. */
      &task_cpu_temp); /* Task handle. */
}

void onWsEvent(AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len)
{
  if (type == WS_EVT_CONNECT)
  {
    Serial.printf("ws[%s][%u] connect\n", server->url(), client->id());
    client->ping();
  }
  else if (type == WS_EVT_DISCONNECT)
  {
    Serial.printf("ws[%s][%u] disconnect: %u\n", server->url(), client->id());
  }
  else if (type == WS_EVT_ERROR)
  {
    Serial.printf("ws[%s][%u] error(%u): %s\n", server->url(), client->id(), *((uint16_t *)arg), (char *)data);
  }
  else if (type == WS_EVT_PONG)
  {
    Serial.printf("ws[%s][%u] pong[%u]: %s\n", server->url(), client->id(), len, (len) ? (char *)data : "");
  }
  else if (type == WS_EVT_DATA)
  {
    AwsFrameInfo *info = (AwsFrameInfo *)arg;
    String msg = "";
    if (info->final && info->index == 0 && info->len == len)
    {
      //the whole message is in a single frame and we got all of it's data
      Serial.printf("ws[%s][%u] %s-message[%llu]: ", server->url(), client->id(), (info->opcode == WS_TEXT) ? "text" : "binary", info->len);

      if (info->opcode == WS_TEXT)
      {
        for (size_t i = 0; i < info->len; i++)
        {
          msg += (char)data[i];
        }
      }
      else
      {
        char buff[3];
        for (size_t i = 0; i < info->len; i++)
        {
          sprintf(buff, "%02x ", (uint8_t)data[i]);
          msg += buff;
        }
      }
      Serial.printf("%s\n", msg.c_str());

      if (info->opcode == WS_TEXT)
        Serial.println("I got your text message");
      else
        Serial.println("I got your binary message");
    }
    else
    {
      //message is comprised of multiple frames or the frame is split into multiple packets
      if (info->index == 0)
      {
        if (info->num == 0)
          Serial.printf("ws[%s][%u] %s-message start\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
        Serial.printf("ws[%s][%u] frame[%u] start[%llu]\n", server->url(), client->id(), info->num, info->len);
      }

      Serial.printf("ws[%s][%u] frame[%u] %s[%llu - %llu]: ", server->url(), client->id(), info->num, (info->message_opcode == WS_TEXT) ? "text" : "binary", info->index, info->index + len);

      if (info->opcode == WS_TEXT)
      {
        for (size_t i = 0; i < len; i++)
        {
          msg += (char)data[i];
        }
      }
      else
      {
        char buff[3];
        for (size_t i = 0; i < len; i++)
        {
          sprintf(buff, "%02x ", (uint8_t)data[i]);
          msg += buff;
        }
      }
      Serial.printf("%s\n", msg.c_str());

      if ((info->index + len) == info->len)
      {
        Serial.printf("ws[%s][%u] frame[%u] end[%llu]\n", server->url(), client->id(), info->num, info->len);
        if (info->final)
        {
          Serial.printf("ws[%s][%u] %s-message end\n", server->url(), client->id(), (info->message_opcode == WS_TEXT) ? "text" : "binary");
          if (info->message_opcode == WS_TEXT)
            client->text("I got your text message");
          else
            client->binary("I got your binary message");
        }
      }
    }
  }
}

void setup()
{
  Serial.begin(115200);
  create_Tasks();

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

  // Websocket
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);

  server.begin();
  server.serveStatic("/", SPIFFS, "/");
}

void loop() {}
