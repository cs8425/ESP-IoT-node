#ifndef __CONFIG_H_
#define __CONFIG_H_

#include <ESP8266WiFi.h>

////////////////////////////////////////////////////////

#define WIFI_MODE       WIFI_AP_STA // WIFI_AP, WIFI_STA, WIFI_AP_STA

#define AP_SSID         "esp"
#define AP_PWD          "Aa123454321aA"
#define AP_CHANNEL      1 // 0~14
#define AP_HIDDEN       0

#define STA_SSID        "your_wifi_SSID"
#define STA_PWD         "password_for_your_wifi"

#define TZ              8       // (utc+) TZ in hours
#define DST_MN          0      // use 60mn for summer time in some countries


#define AES_KEY         "0123456789abcdef0123456789abcdef" // bytes, 32 byte

////////////////////////////////////////////////////////

#define CONFIG_FILE     "/config"
#define KEY_FILE        "/key"

#endif

