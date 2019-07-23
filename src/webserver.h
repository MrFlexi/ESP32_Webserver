#ifndef _WEBSERVER_H
#define _WEBSERVER_H

StaticJsonDocument<500> doc;


typedef struct
{
  String title;
  String description;
  String date;
  String priority;
    
} error_message_t;


typedef struct
{
  int roundtrips;
  float temperatur;
  int32_t latitude;
  int32_t longitude;
  int error_msg_count;  
} message_buffer_t;

 error_message_t* error_tab = new error_message_t[10];


String message_buffer_to_jsonstr(message_buffer_t message_buffer, error_message_t  error_tab[])
{
  String JsonStr;
  doc.clear();

  doc["roundtrips"] = String( message_buffer.roundtrips);
  doc["sensor"] = "gps";
  doc["time"] = "10:05";
  doc["text"] = "Hallo Welt";
  doc["text_time"] = "SA 8:22:01";

  doc["temperatur"] = String( message_buffer.temperatur);

  // Add the "feeds" array
  JsonArray feeds = doc.createNestedArray("text_table");

  Serial.println("46");

  for (int i = 0; i < message_buffer.error_msg_count; i++)
  {
    JsonObject msg = feeds.createNestedObject();
    msg["Title"] = error_tab[i].title;
    Serial.println("52");
    //msg["Description"] = "400m Schwimmen in 4 Minuten";
    //msg["Date"] = "13.10.1972";
    msg["Priority"] = error_tab[i].priority;
    feeds.add(msg);
    Serial.println("57");
  }
  Serial.println("59");
  serializeJson(doc, JsonStr);
  serializeJsonPretty(doc, Serial);
  return JsonStr;
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




#endif