#ifndef __CONFIG_H_
#define __CONFIG_H_

#include <ESP8266WiFi.h>

////////////////////////////////////////////////////////

//#define OUT_PIN D4
#define OUT_PIN LED_BUILTIN // GPIO 2

#define OUT_LOW_ACTIVE  1 // 1 >> low active

#define DEF_ON_TIME     300 // default ON time
#define DEF_OFF_TIME    1500 // default OFF time

#define WIFI_MODE       WIFI_AP_STA // WIFI_AP, WIFI_STA, WIFI_AP_STA

#define AP_SSID         "esp"
#define AP_PWD          "Aa123454321aA"
#define AP_CHANNEL      1 // 1~14
#define AP_HIDDEN       0

#define STA_SSID        "your_wifi_SSID"
#define STA_PWD         "password_for_your_wifi"

#define TZ              8       // (utc+) TZ in hours
#define DST_MN          0      // use 60mn for summer time in some countries


#define AES_KEY         "8d969eef6ecad3c29a3a629280e686cf0c3f5d5a86aff3ca12020c923adc6c92" // hex 64 Bytes, raw 32 bytes, default = sha256("123456")

#define TAG             "node-01"

////////////////////////////////////////////////////////

#define CONFIG_FILE     "/config"
#define KEY_FILE        "/key" // raw bytes
#define SCHEDLE_FILE    "/sch" // raw bytes

#define MAX_SCHEDLE_COUNT 24 // max schedule count
#define MAX_LOG_COUNT (60*24) // max sensor log count

#define PWR_SLEEP_MS    8 // sleep for power saving

#define RES_BUF_SIZE    536 // buffer size for ResponseStream

#endif

