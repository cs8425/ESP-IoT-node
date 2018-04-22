#ifndef __CONFIG_H_
#define __CONFIG_H_

#include <ESP8266WiFi.h>

////////////////////////////////////////////////////////

#define ENABLE_AP       1       // 0: not start AP  1: start AP
#define ENABLE_STA      1       // 0: not start STA  1: start STA

#define AP_SSID         "esp"
#define AP_PWD          "Aa123454321aA"

#define STA_SSID        "your_wifi_SSID"
#define STA_PWD         "password_for_your_wifi"

#define TZ              8       // (utc+) TZ in hours
#define DST_MN          0      // use 60mn for summer time in some countries


#define RTC_TEST     1510592825 // 1510592825 = Monday 13 November 2017 17:07:05 UTC

////////////////////////////////////////////////////////

/*
#if ENABLE_AP && ENABLE_STA
	#define WIFI_MODE WIFI_AP_STA
#endif
*/

#endif

