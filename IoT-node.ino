/*

*/

#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <FS.h>

#include <time.h>                       // time() ctime()
#include <sys/time.h>                   // struct timeval
#include <coredecls.h>                  // settimeofday_cb()

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

//#include "micro-ecc/uECC.h"
#include "config.h"
#include "util.hpp"
#include "schedule.hpp"
#include "sensor_log.hpp"


#define TZ_MN           ((TZ)*60)
#define TZ_SEC          ((TZ)*3600)
#define DST_SEC         ((DST_MN)*60)

bool cbtime_set = false;

void time_is_set (void) {
	cbtime_set = true;
	Serial.println("------------------ settimeofday() was called ------------------");
}


Log logs;

Scheduler sch;
Mode pin;

AsyncWebServer server(80);
//AsyncWebSocket ws("/ws");

void setup() {
	Serial.begin(115200);

	EEPROM.begin(512);

	configTime(TZ_SEC, DST_SEC, "1.tw.pool.ntp.org", "1.asia.pool.ntp.org", "pool.ntp.org");

	// TODO: load from EEPROM
	for(uint8_t i=0; i<7; i++){
		sch.SetDefaultMode(i, i, i+1);
	}

	pin.SetMode(15, 10, true);

	WiFi.persistent(false); // !!! less flash write for WiFiMulti !!!
	WiFi.mode(WIFI_AP_STA);
	WiFi.begin(STA_SSID, STA_PWD);
	WiFi.softAP(AP_SSID, AP_PWD);

	// don't wait, observe time changing when ntp timestamp is received

	settimeofday_cb(time_is_set);
/*
	// settime
	time_t rtc = 1510592825; // 1510592825 = Monday 13 November 2017 17:07:05 UTC
	timeval tv = { rtc, 0 };
	timezone tz = { TZ_MN + DST_MN, 0 };
	settimeofday(&tv, &tz);
*/

	SPIFFS.begin();

	server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *req){
		req->send(200, "text/plain", String(ESP.getFreeHeap()));
	});

	server.serveStatic("/", SPIFFS, "/web/").setDefaultFile("index.html");
	server.begin();
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

timespec tp;
time_t now;
uint32_t now_ms, now_us;
uint32_t last_ms;

void loop() {

	clock_gettime(0, &tp);
	now = time(nullptr);
	now_ms = millis();
	now_us = micros();

/*	if(now_ms - last_ms > 10000){
		scan();
		Serial.print(WiFi.status());
		Serial.println();
		last_ms = now_ms;
	}*/

	if(now_ms - last_ms > 1000){
		Serial.print("cbtime_set = ");
		Serial.println(cbtime_set);

		int sid = sch.Update(now);
		const mode* m = sch.GetOutput();
		Serial.print("output update = ");
		Serial.print(sid);
		Serial.print(", mode.on = ");
		Serial.print(m->on);
		Serial.print(", mode.off = ");
		Serial.println(m->off);

		if (sid) {
			Serial.println("force set output mode !");
			pin.SetMode(m);
		}

		Serial.print("mode output = ");
		Serial.println(pin.Update());


		Serial.print("wifi status: ");
		Serial.print(WiFi.status());
		Serial.print(", wifi SSID: ");
		Serial.print(WiFi.SSID());
		Serial.print(", IP Address: ");
		Serial.println(WiFi.localIP());

		Serial.print("Free heap:");
		Serial.println(ESP.getFreeHeap(), DEC);
		Serial.println();

		Serial.println();
		printTm("localtime", localtime(&now));
		Serial.println();
		printTm("gmtime   ", gmtime(&now));

		// time from boot
		Serial.print("clock:");
		Serial.print((uint32_t)tp.tv_sec);
		Serial.print("/");
		Serial.print((uint32_t)tp.tv_nsec);
		Serial.print("ns");
		Serial.println();
		Serial.println();


		logs.Add(tp.tv_sec, tp.tv_nsec);

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


