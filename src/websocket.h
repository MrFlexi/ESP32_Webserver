#ifndef _WEBSOCKET_H
#define _WEBSOCKET_H

#include <ArduinoJson.h>

StaticJsonDocument<500> doc;
// local Tag for logging

 extern AsyncWebSocket ws;

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
  

  for (int i = 0; i < message_buffer.error_msg_count; i++)
  {
    JsonObject msg = feeds.createNestedObject();
    msg["Title"] = error_tab[i].title;
    //msg["Description"] = "400m Schwimmen in 4 Minuten";
    //msg["Date"] = "13.10.1972";
    msg["Priority"] = error_tab[i].priority;
    feeds.add(msg);    
  }  
  serializeJson(doc, JsonStr);
  serializeJsonPretty(doc, Serial);
  return JsonStr;
}

void t_broadcast_message(void *parameter)
{
  // Task bound to core 0, Prio 0 =  very low

  error_message_t error_message;

  String JsonStr;
  bool sendMessage = false;

  for (;;)
  {
    
    // Check if values have been changed
    if ( gs_message_buffer.temperatur != gs_message_buffer_old.temperatur or
        gs_message_buffer.roundtrips != gs_message_buffer_old.roundtrips )
    {
     gs_message_buffer_old =  gs_message_buffer;
     sendMessage = true;
    }
    
    // Check if there is a new queue entry to display in terminal
    if (queue != NULL)
    {

      int messagesWaiting = uxQueueMessagesWaiting(queue);
    

      gs_message_buffer.error_msg_count = messagesWaiting;

      if (messagesWaiting > 0)
      {
        
        Serial.print("Messages waiting: ");
        Serial.println(messagesWaiting);
        sendMessage = true;

        for (int i = 0; i < messagesWaiting; i++)
        {

          xQueueReceive(queue, &error_message, portMAX_DELAY);
          Serial.print(error_message.priority);
          Serial.print("|");
          Serial.println(error_message.title);

          // Put into array
          error_tab[i].priority = error_message.priority;
          error_tab[i].title = error_message.title;
        }
      }

      // Send message via Websocket to connected clients
      if (sendMessage)
      {
        //JsonStr = message_buffer_to_jsonstr(gs_message_buffer, error_tab);
        ws.textAll(JsonStr);
        sendMessage = false;
      }
    }

    delay(100);
  }
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