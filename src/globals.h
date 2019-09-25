#ifndef _GLOBALS_H
#define _GLOBALS_H

#include "hal/ttgobeam.h"

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

 extern error_message_t* error_tab = new error_message_t[10];

 extern message_buffer_t gs_message_buffer;
 extern message_buffer_t gs_message_buffer_old;
 extern message_buffer_t gs_message_queue_out;
 extern QueueHandle_t queue;
 extern int queueSize;

 extern error_message_t gs_error_message;

#define U8LOG_WIDTH 64
#define U8LOG_HEIGHT 4
#define SUN	0
#define SUN_CLOUD  1
#define CLOUD 2
#define RAIN 3
#define THUNDER 4
#define SLEEP 10


#endif