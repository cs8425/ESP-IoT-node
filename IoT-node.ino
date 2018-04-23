/*

*/

#include <ESP8266WiFi.h>
#include <EEPROM.h>
#include <FS.h>

#include <time.h>                       // time() ctime()
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
	Serial.println("\n------------------ settimeofday() was called ------------------");
}


Log logs;

Scheduler sch;
Mode pin;

AsyncWebServer server(80);

#define WERR(req) {\
	req->send(500); \
	return; }

bool authCheck(AsyncWebServerRequest *req, String& val) {
	if(!req->hasParam("key")) {
		req->send(403);
		return false;
	}

	String key = req->getParam("key")->value();
	if (key != val) {
		return false;
	}
	return true;
}

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


	SPIFFS.begin();

	server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *req){
		req->send(200, "text/plain", String(ESP.getFreeHeap()));
	});

	server.on("/sch/ls", HTTP_GET, [](AsyncWebServerRequest *req){
		AsyncResponseStream *res = req->beginResponseStream("text/plain");
		for(unsigned i = 0; i < 7; i++){
			const mode* m = sch.GetDef(i);
			res->printf("%u:%u,%u\n", i, m->on, m->off);
		}

		unsigned count = sch.Count();
		for(unsigned i = 0; i < count; i++){
			const schedule* data = sch.Get(i);
			res->printf("%u,%u,%u,%u,%u\n", data->start.week, data->start.sec, data->end.sec, data->m.on, data->m.off);
		}

		req->send(res);
	});

	server.on("/sch/add", HTTP_GET, [](AsyncWebServerRequest *req){
		int params = req->params();
		if (params != 5) WERR(req);
		if(!req->hasParam("w")) WERR(req);
		if(!req->hasParam("as")) WERR(req);
		if(!req->hasParam("bs")) WERR(req);
		if(!req->hasParam("on")) WERR(req);
		if(!req->hasParam("of")) WERR(req);

		unsigned w = unsigned(req->getParam("w")->value().toInt());
		unsigned as = unsigned(req->getParam("as")->value().toInt());
		unsigned bs = unsigned(req->getParam("bs")->value().toInt());
		unsigned on = unsigned(req->getParam("on")->value().toInt());
		unsigned of = unsigned(req->getParam("of")->value().toInt());

		int idx = sch.Add(daytime{w, as}, daytime{w, bs}, mode{on, of});
		req->send(200, "text/plain", String(idx));
	});

	server.on("/sch/rm", HTTP_GET, [](AsyncWebServerRequest *req){
		int params = req->params();
		if (params != 1) WERR(req);
		if(!req->hasParam("i")) WERR(req);

		unsigned idx = unsigned(req->getParam("i")->value().toInt());
		req->send(200, "text/plain", String(sch.Del(idx)));
	});

	server.on("/mode", HTTP_GET, [](AsyncWebServerRequest *req){
		if(!req->hasParam("on")) WERR(req);
		if(!req->hasParam("off")) WERR(req);

		String on = req->getParam("on")->value();
		String off = req->getParam("off")->value();
		pin.SetMode(on.toInt(), off.toInt());

		req->send(200, "text/plain", String(ESP.getFreeHeap()));
	});

	server.on("/log", HTTP_GET, [](AsyncWebServerRequest *req){
		AsyncResponseStream *res = req->beginResponseStream("text/plain");
		unsigned count = logs.Count();
		for(unsigned i = 0; i < count; i++){
			const log_t* data = logs.Get(i);
			res->printf("%u,%d,%d\n", i, data->temp, data->hum);
		}

		req->send(res);
	});

	// test return only
	server.on("/test", HTTP_GET, [](AsyncWebServerRequest *req){
		WERR(req);
	});

	// for debug
	server.on("/out", HTTP_GET, [](AsyncWebServerRequest *req){
		AsyncResponseStream *res = req->beginResponseStream("text/plain");
		unsigned count = sch.Count();
		const mode* m = sch.GetOutput();
		res->printf("sch: %u,%u,%u\n", count, m->on, m->off);
		res->printf("pin: %u\n", pin.GetOutput());
		req->send(res);
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

time_t now;
uint32_t now_ms, now_us;
uint32_t last_ms;

void loop() {

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
		last_ms = now_ms;

		Serial.print("cbtime_set = ");
		Serial.println(cbtime_set);

		Serial.print("now_ms = ");
		Serial.println(now_ms);

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


		logs.Add(ESP.getFreeHeap(), sid);
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


