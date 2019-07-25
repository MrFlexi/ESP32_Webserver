#include <Arduino.h>
#include "WiFi.h"
#include "SPIFFS.h"
#include "ESPAsyncWebServer.h"
#include <ArduinoJson.h>
#include "esp_system.h"
#include "webserver.h"
#include "tasks.h"
#include <Wire.h>
#include <SPI.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>

#define BME_SCK 13
#define BME_MISO 12
#define BME_MOSI 11
#define BME_CS 10

#define SEALEVELPRESSURE_HPA (1013.25)


//Onboard OLE Display
//Pin 4 = SDA (ist nicht der Standard-I2C-Port vom ESP32)
//Pin 15 = SCL (ist nicht der Standard-I2C-Port vom ESP32)
//Pin 16 = RST (muss bei Start kurz auf Low, dann auf High gesetzt werden)

//U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ 15, /* data=*/ 4, /* reset=*/ 16);


Adafruit_BME280 bme; // I2C   PIN 21 + 22

//-----------------------------------------------------------------------
// RTOS
//-----------------------------------------------------------------------
TaskHandle_t task_alive_msg;
TaskHandle_t task_cpu_temp;
TaskHandle_t task_broadcast_message;
QueueHandle_t queue;

const char *ssid = "MrFlexi";
const char *password = "Linde-123";
const float alive_msg_intervall = 20; // 60 seconds
int roundtrips = 0;
int queueSize = 10;

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

extern "C"
{
  uint8_t temprature_sens_read();
}

message_buffer_t gs_message_buffer;
message_buffer_t gs_message_buffer_old;
message_buffer_t gs_message_queue_out;

error_message_t gs_error_message;

void t_cpu_temp(void *parameter)
{

  for (;;)
  {
    //get internal temp of ESP32
    uint8_t temp_farenheit = temprature_sens_read();
    //convert farenheit to celcius
    gs_message_buffer.temperatur = (temp_farenheit - 32) / 1.8;

    Serial.print("internal temp [°C]: ");
    Serial.println(gs_message_buffer.temperatur);
    delay(60000);
  }
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
    delay(20000);
  }
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
        JsonStr = message_buffer_to_jsonstr(gs_message_buffer, error_tab);
        ws.textAll(JsonStr);
        sendMessage = false;
      }
    }

    delay(100);
  }
}

void create_Tasks()
{

  xTaskCreate(
      t_broadcast_message,      /* Task function. */
      "Broadcast Message",      /* String with name of task. */
      10000,                    /* Stack size in bytes. */
      NULL,                     /* Parameter passed as input of the task */
      10,                       /* Priority of the task. */
      &task_broadcast_message); /* Task handle. */

  xTaskCreate(
      t_alive_msg,      /* Task function. */
      "AliveMessage",   /* String with name of task. */
      10000,            /* Stack size in bytes. */
      NULL,             /* Parameter passed as input of the task */
      0,                /* Priority of the task. */
      &task_alive_msg); /* Task handle. */

  xTaskCreate(
      t_cpu_temp,      /* Task function. */
      "CPUTemp",       /* String with name of task. */
      10000,           /* Stack size in bytes. */
      NULL,            /* Parameter passed as input of the task */
      0,               /* Priority of the task. */
      &task_cpu_temp); /* Task handle. */
}


void setup_sensors()
{
  Serial.println(F("BME280 test")); 
     unsigned status; 
     
     
    // https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/issues/62

    bool wire_status = Wire1.begin(21, 22);
    if(!wire_status)
    {
      Serial.println("Could not finitialize Wire1"); 
    }


     status = bme.begin(0x76, &Wire1);  
     if (!status) { 
         Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!"); 
         Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(),16); 
         Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n"); 
         Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n"); 
         Serial.print("        ID of 0x60 represents a BME 280.\n"); 
         Serial.print("        ID of 0x61 represents a BME 680.\n"); 
         while (1); 
     } 
     
     Serial.println(); 
     Serial.print("Temperature = "); 
     Serial.print(bme.readTemperature()); 
     Serial.println(" *C");  
     Serial.print("Pressure = "); 
     Serial.print(bme.readPressure() / 100.0F); 
     Serial.println(" hPa");   
     Serial.print("Approx. Altitude = "); 
     Serial.print(bme.readAltitude(SEALEVELPRESSURE_HPA)); 
     Serial.println(" m");   
     Serial.print("Humidity = "); 
     Serial.print(bme.readHumidity()); 
     Serial.println(" %");  
     Serial.println(); 
  }





void setup()
{
  Serial.begin(115200);
  setup_sensors();

  // RTOS Initialisation
  queue = xQueueCreate(queueSize, sizeof(error_message_t));
  if (queue == NULL)
  {
    Serial.println("Error creating the queue");
  }

  gs_error_message.priority = "High";
  gs_error_message.title = "CPU temperatur high";
  xQueueSend(queue, &gs_error_message, portMAX_DELAY);

  gs_error_message.priority = "Low";
  gs_error_message.title = "Lora Data received";
  xQueueSend(queue, &gs_error_message, portMAX_DELAY);

  create_Tasks();

  // External File System Initialisation
  if (!SPIFFS.begin())
  {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // WIFI Setup
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
