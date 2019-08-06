// clang-format off
// upload_speed 115200
// board ttgo-lora32-v1

#ifndef _TTGOV1_H
#define _TTGOV1_H

#include <stdint.h>

// Hardware related definitions for TTGOv1 board

#define HAS_LORA 1       // comment out if device shall not send data via LoRa
#define CFG_sx1276_radio 1

// GPS settings
#define HAS_GPS 1 // use on board GPS
#define GPS_SERIAL 9600, SERIAL_8N1, GPIO_NUM_12, GPIO_NUM_1 // GPIO12 = RX
//#define GPS_INT GPIO_NUM_13 // 30ns accurary timepulse, to be external wired o


#define HAS_BME280 GPIO_NUM_21, GPIO_NUM_22 // SDA, SCL

#define HAS_DISPLAY U8X8_SSD1306_128X64_NONAME_HW_I2C // OLED-Display on board
//#define DISPLAY_FLIP  1 // uncomment this for rotated display
#define HAS_LED LED_BUILTIN
#define LED_ACTIVE_LOW 1  // Onboard LED is active when pin is LOW
#define HAS_BUTTON KEY_BUILTIN

// Pins for I2C interface of OLED Display
#define MY_OLED_SDA (4)
#define MY_OLED_SCL (15)
#define MY_OLED_RST (16)

// Pins for LORA chip SPI interface come from board file, we need some
// additional definitions for LMIC
#define LORA_IO1  (33)
#define LORA_IO2  LMIC_UNUSED_PIN

#endif
