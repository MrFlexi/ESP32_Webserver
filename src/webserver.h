#ifndef _WEBSERVER_H
#define _WEBSERVER_H

StaticJsonDocument<500> doc;


typedef struct
{
  int roundtrips;
  float temperatur;
  int32_t latitude;
  int32_t longitude;
} message_buffer_t;

String message_buffer_to_jsonstr(message_buffer_t message_buffer)
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


#endif