#define USE_WEBSERVER   0
#define USE_WEBSOCKET   0
#define USE_WIFI        1
#define USE_BME280      1

#include <Arduino.h>
#include "WiFi.h"
#include "esp_system.h"
#include "tasks.h"
#include <Wire.h>
#include <SPI.h>
#include <U8g2lib.h>
#include <ArduinoJson.h>
#include <esp_spi_flash.h> // needed for reading ESP32 chip attributes

#include "globals.h"


#if (USE_WEBSERVER)
#include "SPIFFS.h"
#include "ESPAsyncWebServer.h"
#include "webserver.h"
#endif

#define SUN	0
#define SUN_CLOUD  1
#define CLOUD 2
#define RAIN 3
#define THUNDER 4
#define SLEEP 10


//--------------------------------------------------------------------------
// U8G2 Display Setup
//--------------------------------------------------------------------------
//U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ U8X8_PIN_NONE, /* clock=*/ OLED_SCL, /* data=*/ OLED_SDA);   // ESP32 Thing, HW I2C with pin remapping

U8G2_SSD1306_128X64_NONAME_F_HW_I2C u8g2(U8G2_R0, /* reset=*/ OLED_RST, /* clock=*/ OLED_SCL, /* data=*/ OLED_SDA);   // ESP32 Thing, HW I2C with pin remapping


// Create a U8g2log object
U8G2LOG u8g2log;

// assume 4x6 font, define width and height
#define U8LOG_WIDTH 32
#define U8LOG_HEIGHT 6

// allocate memory
uint8_t u8log_buffer[U8LOG_WIDTH * U8LOG_HEIGHT];


#if (USE_WEBSOCKET)
#include "ESPAsyncWebServer.h"
#include "websocket.h"
#endif


#if (USE_BME280)
#include <Adafruit_Sensor.h>
#include <Adafruit_BME280.h>
#endif




#define SEALEVELPRESSURE_HPA (1013.25)


//Onboard OLE Display
//Pin 4 = SDA (ist nicht der Standard-I2C-Port vom ESP32)
//Pin 15 = SCL (ist nicht der Standard-I2C-Port vom ESP32)
//Pin 16 = RST (muss bei Start kurz auf Low, dann auf High gesetzt werden)

//U8G2_SSD1306_128X64_NONAME_F_SW_I2C u8g2(U8G2_R0, /* clock=*/ 15, /* data=*/ 4, /* reset=*/ 16);


Adafruit_BME280 bme; // I2C   PIN 21 + 22

// local Tag for logging
static const char TAG[] = __FILE__;

//-----------------------------------------------------------------------
// RTOS
//-----------------------------------------------------------------------
TaskHandle_t task_alive_msg;
TaskHandle_t task_sensors;
TaskHandle_t task_broadcast_message;

int queueSize = 10;

const char *ssid = "MrFlexi";
const char *password = "Linde-123";
const float alive_msg_intervall = 20; // 60 seconds
int roundtrips = 0;

// Global Variables - Definition checked against declaration */
message_buffer_t gs_message_buffer;
message_buffer_t gs_message_buffer_old;
message_buffer_t gs_message_queue_out;
QueueHandle_t queue;
error_message_t gs_error_message;





#if (USE_WEBSERVER)
AsyncWebServer server(80);
#endif

#if (USE_WEBSOCKET)
AsyncWebSocket ws("/ws");
#endif

extern "C"
{
  uint8_t temprature_sens_read();
}


void t_sensors(void *parameter)
{

  for (;;)
  {
    //get internal temp of ESP32
    uint8_t temp_farenheit = temprature_sens_read();
    //convert farenheit to celcius
    gs_message_buffer.temperatur = (temp_farenheit - 32) / 1.8;

    Serial.print("CPU temp [°C]: ");
    Serial.println(gs_message_buffer.temperatur);


     Serial.println(); 
     Serial.print("Temp [°C]: "); 
     Serial.print(bme.readTemperature()); 
     Serial.println();
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
    delay(10000);
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



void create_Tasks()
{

#if (USE_WEBSOCKET)
  xTaskCreate(
      t_broadcast_message,      /* Task function. */
      "Broadcast Message",      /* String with name of task. */
      10000,                    /* Stack size in bytes. */
      NULL,                     /* Parameter passed as input of the task */
      10,                       /* Priority of the task. */
      &task_broadcast_message); /* Task handle. */
#endif      

  xTaskCreate(
      t_alive_msg,      /* Task function. */
      "AliveMessage",   /* String with name of task. */
      10000,            /* Stack size in bytes. */
      NULL,             /* Parameter passed as input of the task */
      0,                /* Priority of the task. */
      &task_alive_msg); /* Task handle. */

  xTaskCreate(
      t_sensors,      /* Task function. */
      "Sensors",       /* String with name of task. */
      10000,           /* Stack size in bytes. */
      NULL,            /* Parameter passed as input of the task */
      0,               /* Priority of the task. */
      &task_sensors); /* Task handle. */
}



void drawScrollString(int16_t offset, const char *s)
{
  static char buf[36];	// should for screen with up to 256 pixel width 
  size_t len;
  size_t char_offset = 0;
  u8g2_uint_t dx = 0;
  size_t visible = 0;
  len = strlen(s);
  if ( offset < 0 )
  {
    char_offset = (-offset)/8;
    dx = offset + char_offset*8;
    if ( char_offset >= u8g2.getDisplayWidth()/8 )
      return;
    visible = u8g2.getDisplayWidth()/8-char_offset+1;
    strncpy(buf, s, visible);
    buf[visible] = '\0';
    u8g2.setFont(u8g2_font_8x13_mf);
    u8g2.drawStr(char_offset*8-dx, 62, buf);
  }
  else
  {
    char_offset = offset / 8;
    if ( char_offset >= len )
      return;	// nothing visible
    dx = offset - char_offset*8;
    visible = len - char_offset;
    if ( visible > u8g2.getDisplayWidth()/8+1 )
      visible = u8g2.getDisplayWidth()/8+1;
    strncpy(buf, s+char_offset, visible);
    buf[visible] = '\0';
    u8g2.setFont(u8g2_font_8x13_mf);
    u8g2.drawStr(-dx, 62, buf);
  }
  
}


void drawSymbol(u8g2_uint_t x, u8g2_uint_t y, uint8_t symbol)
{
  // fonts used:
  // u8g2_font_open_iconic_embedded_6x_t
  // u8g2_font_open_iconic_weather_6x_t
  // encoding values, see: https://github.com/olikraus/u8g2/wiki/fntgrpiconic
  
  switch(symbol)
  {
    case SUN:
      u8g2.setFont(u8g2_font_open_iconic_weather_6x_t);
      u8g2.drawGlyph(x, y, 69);	
      break;
    case SUN_CLOUD:
      u8g2.setFont(u8g2_font_open_iconic_weather_6x_t);
      u8g2.drawGlyph(x, y, 65);	
      break;
    case CLOUD:
      u8g2.setFont(u8g2_font_open_iconic_weather_6x_t);
      u8g2.drawGlyph(x, y, 64);	
      break;
    case RAIN:
      u8g2.setFont(u8g2_font_open_iconic_weather_6x_t);
      u8g2.drawGlyph(x, y, 67);	
      break;    
    case THUNDER:
      u8g2.setFont(u8g2_font_open_iconic_embedded_6x_t);
      u8g2.drawGlyph(x, y, 67);
      break;
   case SLEEP:
      u8g2.setFont( u8g2_font_open_iconic_all_8x_t);
      u8g2.drawGlyph(x, y, 67);  
      break;         
  }
}

void drawWeather(uint8_t symbol, int degree)
{
  drawSymbol(0, 48, symbol);
  u8g2.setFont(u8g2_font_logisoso32_tf);
  u8g2.setCursor(48+3, 42);
  u8g2.print(degree);
  u8g2.print("°C");		// requires enableUTF8Print()
}

void drawRawValue(uint8_t symbol, int degree, int voltage)
{
  Serial.print("drawRawValue");
  u8g2.firstPage();
  drawSymbol(0, 48, symbol);
  u8g2.setFont(u8g2_font_logisoso16_tf);
  u8g2.setCursor(48+3, 20);
  u8g2.print(degree);u8g2.print(" °");
  u8g2.print("");	
  
  u8g2.setCursor(48+3, 42);
  u8g2.print(voltage);u8g2.print(" V");
  u8g2.print("");	

  while ( u8g2.nextPage() );
  delay(10);
}


void draw(const char *s, uint8_t symbol, int degree)
{
  int16_t offset = -(int16_t)u8g2.getDisplayWidth();
  int16_t len = strlen(s);
  for(;;)
  {
    u8g2.firstPage();
    do {
      drawWeather(symbol, degree);
      drawScrollString(offset, s);
    } while ( u8g2.nextPage() );
    delay(5);
    offset+=2;
    if ( offset > len*8+1 )
      break;
  }
}





void print_wakeup_reason()
{
  esp_sleep_wakeup_cause_t wakeup_reason;

  wakeup_reason = esp_sleep_get_wakeup_cause();

  switch (wakeup_reason)
  {
    case ESP_SLEEP_WAKEUP_EXT0:
      Serial.println("Wakeup caused by external signal using RTC_IO");
      break;
    case ESP_SLEEP_WAKEUP_EXT1:
      Serial.println("Wakeup caused by external signal using RTC_CNTL");
      break;
    case ESP_SLEEP_WAKEUP_TIMER:
      Serial.println("Wakeup caused by timer");
      break;
    case ESP_SLEEP_WAKEUP_TOUCHPAD:
      Serial.println("Wakeup caused by touchpad");
      break;
    case ESP_SLEEP_WAKEUP_ULP:
      Serial.println("Wakeup caused by ULP program");
      break;
    default:
      Serial.printf("Wakeup was not caused by deep sleep: %d\n", wakeup_reason);
      break;
  }
}



void setup_display(void)
{
  u8g2.begin();
  u8g2.setFont(u8g2_font_profont12_mf);                         // set the font for the terminal window
  u8g2log.begin(u8g2, U8LOG_WIDTH, U8LOG_HEIGHT, u8log_buffer); // connect to u8g2, assign buffer
  u8g2log.setLineHeightOffset(0);                               // set extra space between lines in pixel, this can be negative
  u8g2log.setRedrawMode(0);                                     // 0: Update screen with newline, 1: Update screen for every char
  u8g2log.print("Display loaded...");
  u8g2log.print("\n");
}

void setup_sensors()
{

 #if (USE_BME280) 
  ESP_LOGI(TAG, "BME280 Setup..."); 

  u8g2log.print("BME280 Setup...");
  u8g2log.print("\n");

     unsigned status;      
     
    // https://github.com/Heltec-Aaron-Lee/WiFi_Kit_series/issues/62

    bool wire_status = Wire1.begin( GPIO_NUM_21, GPIO_NUM_13);
    if(!wire_status)
    {
      ESP_LOGE(TAG, "I2C Wire1 error");; 
      u8g2log.print("Wire1 error");
      u8g2log.print("\n");
    }


     status = bme.begin(0x76, &Wire1);  
     //status = bme.begin(0x76); 
     if (!status) { 
         Serial.println("Could not find a valid BME280 sensor, check wiring, address, sensor ID!"); 
         Serial.print("SensorID was: 0x"); Serial.println(bme.sensorID(),16); 
         Serial.print("        ID of 0xFF probably means a bad address, a BMP 180 or BMP 085\n"); 
         Serial.print("   ID of 0x56-0x58 represents a BMP 280,\n"); 
         Serial.print("        ID of 0x60 represents a BME 280.\n"); 
         Serial.print("        ID of 0x61 represents a BME 680.\n"); 
         while (1); 
     }     
     
#endif

  }

void DisplayKey(const uint8_t *key, uint8_t len, bool lsb) {
  const uint8_t *p;
  for (uint8_t i = 0; i < len; i++) {
    p = lsb ? key + len - i - 1 : key + i;
    u8g2.printf("%02X", *p);
  }
  u8g2.printf("\n");
}

void display_chip_info()
{
    esp_chip_info_t chip_info;

    esp_chip_info(&chip_info);
    u8g2.printf("ESP32 %d cores\nWiFi%s%s\n", chip_info.cores,
                (chip_info.features & CHIP_FEATURE_BT) ? "/BT" : "",
                (chip_info.features & CHIP_FEATURE_BLE) ? "/BLE" : "");
    u8g2.printf("ESP Rev.%d\n", chip_info.revision);
    u8g2.printf("%dMB %s Flash\n", spi_flash_get_chip_size() / (1024 * 1024),
                (chip_info.features & CHIP_FEATURE_EMB_FLASH) ? "int."
                                                              : "ext.");


}



void setup()
{
  Serial.begin(115200);
  esp_log_level_set("*", ESP_LOG_VERBOSE);

  delay(1000);
  ESP_LOGI(TAG, "Starting..."); 
  
  setup_display();    
  setup_sensors();


  // RTOS Initialisation
  ESP_LOGI(TAG, "creating RTOS Queue"); 
  queue = xQueueCreate(queueSize, sizeof(error_message_t));
  if (queue == NULL)
  {
    ESP_LOGE(TAG, "Error creating the queue"); 
  }

  gs_error_message.priority = "High";
  gs_error_message.title = "CPU temperatur high";
  xQueueSend(queue, &gs_error_message, portMAX_DELAY);

  gs_error_message.priority = "Low";
  gs_error_message.title = "Lora Data received";
  xQueueSend(queue, &gs_error_message, portMAX_DELAY);


  ESP_LOGI(TAG, "creating RTOS Tasks"); 
  create_Tasks();

  #if (USE_WEBSERVER) 
  ESP_LOGI(TAG, "Mounting SPIFF Filesystem"); 
  // External File System Initialisation
  if (!SPIFFS.begin())
  {
    ESP_LOGE(TAG, "An Error has occurred while mounting SPIFFS");     
    return;
  }
  #endif


#if (USE_WIFI) 
  // WIFI Setup
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
      ESP_LOGI(TAG, "Connecting to WiFi..");    
  }

  ESP_LOGI(TAG, WiFi.localIP() );  
  
#endif

#if (USE_WEBSERVER)
  server.on("/index", HTTP_GET, [](AsyncWebServerRequest *request) {
    Serial.println("Index requested");
    request->send(SPIFFS, "/index.html", "text/html");
  });
  server.begin();
  server.serveStatic("/", SPIFFS, "/");
#endif

#if (USE_WEBSOCKET)
  // Websocket
  ws.onEvent(onWsEvent);
  server.addHandler(&ws);
#endif  


draw("Moving solar panel", SUN, 0);
display_chip_info();


}

void loop() {

}
