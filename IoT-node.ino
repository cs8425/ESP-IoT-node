/*

*/

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <time.h>                       // time() ctime()
#include <sys/time.h>                   // struct timeval
#include <coredecls.h>                  // settimeofday_cb()

#include "micro-ecc/uECC.h"
#include "util.hpp"
#include "schedule.hpp"
#include "sensor_log.hpp"

////////////////////////////////////////////////////////

#define SSIDAP          "esp"
#define SSIDPWD         "Aa123454321aA"

#define TZ              8       // (utc+) TZ in hours
#define DST_MN          0      // use 60mn for summer time in some countries

#define NTP0_OR_LOCAL1  0       // 0:use NTP  1:fake external RTC
#define RTC_TEST     1510592825 // 1510592825 = Monday 13 November 2017 17:07:05 UTC

////////////////////////////////////////////////////////

#define TZ_MN           ((TZ)*60)
#define TZ_SEC          ((TZ)*3600)
#define DST_SEC         ((DST_MN)*60)

timeval cbtime;			// time set in callback
bool cbtime_set = false;

void time_is_set (void) {
	gettimeofday(&cbtime, NULL);
	cbtime_set = true;
	Serial.println("------------------ settimeofday() was called ------------------");
}

Log logs;

ESP8266WiFiMulti wifiMulti;

void setup() {
	Serial.begin(115200);

	WiFi.persistent(false); // !!! less flash write for WiFiMulti !!!

	configTime(TZ_SEC, DST_SEC, "pool.ntp.org");
	//WiFi.mode(WIFI_STA);
	//WiFi.begin(SSIDAP, SSIDPWD);

	WiFi.mode(WIFI_AP_STA);
	wifiMulti.addAP("ssid_from_AP_1", "your_password_for_AP_1");
	wifiMulti.addAP("ssid_from_AP_2", "your_password_for_AP_2");
	//wifiMulti.addAP("ssid_from_AP_3", "your_password_for_AP_3");

	WiFi.softAP(SSIDAP, SSIDPWD);

	// don't wait, observe time changing when ntp timestamp is received

/*
	settimeofday_cb(time_is_set);

	// settime
	ESP.eraseConfig();
	time_t rtc = 1510592825; // 1510592825 = Monday 13 November 2017 17:07:05 UTC
	timeval tv = { rtc, 0 };
	timezone tz = { TZ_MN + DST_MN, 0 };
	settimeofday(&tv, &tz);
*/
/*
	if(wifiMulti.run() != WL_CONNECTED) {
		Serial.println("WiFi not connected!");
		delay(1000);
	}
*/
}


#define PTM(w) \
  Serial.print(":" #w "="); \
  Serial.print(tm->tm_##w);

void printTm (const char* what, const tm* tm) {
	Serial.print(what);
	PTM(isdst); PTM(yday); PTM(wday);
	PTM(year);  PTM(mon);  PTM(mday);
	PTM(hour);  PTM(min);  PTM(sec);
}

timeval tv;
timespec tp;
time_t now;
uint32_t now_ms, now_us;
uint32_t last_ms;

void loop() {

	gettimeofday(&tv, nullptr);
	clock_gettime(0, &tp);
	now = time(nullptr);
	now_ms = millis();
	now_us = micros();

	// localtime / gmtime every second change
	static time_t lastv = 0;
	if (lastv != tv.tv_sec) {
		lastv = tv.tv_sec;
		Serial.println();
		printTm("localtime", localtime(&now));
		Serial.println();
		printTm("gmtime   ", gmtime(&now));

		// human readable
		Serial.print(" ctime:(UTC+");
		Serial.print((uint32_t)(TZ * 60 + DST_MN));
		Serial.print("mn)");
		Serial.print(ctime(&now));
		Serial.println();

		// time from boot
		Serial.print("clock:");
		Serial.print((uint32_t)tp.tv_sec);
		Serial.print("/");
		Serial.print((uint32_t)tp.tv_nsec);
		Serial.print("ns");
		Serial.println();

		Serial.print("Free heap:"); 
		Serial.println(ESP.getFreeHeap(), DEC);
		Serial.println();

		logs.Add(tp.tv_sec, tp.tv_nsec);
	}

	if(now_ms - last_ms > 10000){
		scan();
		Serial.print(WiFi.status());
		Serial.println();
		last_ms = now_ms;
	}

}


void scan() {
	// WiFi.scanNetworks will return the number of networks found
	int n = WiFi.scanNetworks();
	Serial.println("scan done");
	if (n == 0) {
		Serial.println("no networks found...");
	} else {
		Serial.print(n);
		Serial.println(" networks found");
		for (int i = 0; i < n; ++i) {
			// Print SSID and RSSI for each network found
			Serial.print(i + 1);
			Serial.print(": ");
			Serial.print(WiFi.SSID(i));
			Serial.print(" (");
			Serial.print(WiFi.RSSI(i));
			Serial.print(")");
			Serial.println((WiFi.encryptionType(i) == ENC_TYPE_NONE)?" ":"*");
		}
	}
	Serial.println("");
}


