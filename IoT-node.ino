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

#include "web.hpp"

//#define OUT_PIN D4
#define OUT_PIN LED_BUILTIN // GPIO 2

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

void setup() {
	Serial.begin(115200);

	EEPROM.begin(512);

	configTime(TZ_SEC, DST_SEC, "1.tw.pool.ntp.org", "1.asia.pool.ntp.org", "pool.ntp.org");

	// TODO: load from EEPROM
	for(uint8_t i=0; i<7; i++){
		sch.SetDefaultMode(i, i, i+1);
	}

	pin.SetMode(15, 10, true);

	pinMode(OUT_PIN, OUTPUT);

	WiFi.persistent(false); // !!! less flash write for WiFiMulti !!!
	WiFi.mode(WIFI_AP_STA);
	WiFi.begin(STA_SSID, STA_PWD);
	WiFi.softAP(AP_SSID, AP_PWD);

	// don't wait, observe time changing when ntp timestamp is received

	settimeofday_cb(time_is_set);


	SPIFFS.begin();

	setupServer(server);

	server.on("/heap", HTTP_GET, [](AsyncWebServerRequest *req){
		req->send(200, "text/plain", String(ESP.getFreeHeap()));
	});

	// for debug
	server.on("/dump", HTTP_GET, [](AsyncWebServerRequest *req){
		AsyncResponseStream *res = req->beginResponseStream("text/plain");
		unsigned count = sch.Count();
		const mode* m = sch.GetOutput();
		int16_t mid = sch.GetModeId();
		res->printf("sch:%d,%u,%u,%u\n", mid, count, m->on, m->off);

		//res->printf("tsk:%u\n", tsk.GetTaskId());

		const log_t* data = logs.GetLatest(1);
		if (data != nullptr) {
			res->printf("sen:%d,%d\n", data->temp, data->hum);
		}

		res->printf("pin:%u\n", pin.GetOutput());
		res->printf("log:%u\n", logs.Count());
		res->printf("heap:%u\n", ESP.getFreeHeap());

		req->send(res);
	});

	server.serveStatic("/", SPIFFS, "/web/").setDefaultFile("index.html");

	server.onNotFound([](AsyncWebServerRequest *request){
		request->send(404);
	});

	DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
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

		//Serial.print("cbtime_set = ");
		//Serial.println(cbtime_set);

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

		int o = pin.Update();
		if (o != -1) {
			digitalWrite(OUT_PIN, o);
		}


		Serial.print("log count = ");
		Serial.println(logs.Count());
		Serial.print("mode output = ");
		Serial.println(pin.GetOutput());


		Serial.print("wifi status: ");
		Serial.print(WiFi.status());
		Serial.print(", wifi SSID: ");
		Serial.print(WiFi.SSID());
		Serial.print(", IP Address: ");
		Serial.println(WiFi.localIP());

		Serial.print("Free heap:");
		Serial.println(ESP.getFreeHeap(), DEC);

		printTm("localtime", localtime(&now));
		Serial.println();
		printTm("gmtime   ", gmtime(&now));
		Serial.println("\n\n");


		//logs.Add(ESP.getFreeHeap() / 10, pin.GetOutput());
		logs.Add(ESP.getFreeHeap() / 10, now_ms / 1000);
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

void setupServer(AsyncWebServer& server) {

/*
	// get & processing setting
	server.on("/setting", HTTP_GET, [](AsyncWebServerRequest *req){

	});
	server.on("/setting", HTTP_POST, [](AsyncWebServerRequest *req){

	});*/

	server.on("/status", HTTP_GET, [](AsyncWebServerRequest *req){
		AsyncResponseStream *res = req->beginResponseStream("text/json");
		unsigned count = sch.Count();
		const mode* m = sch.GetOutput();
		int16_t mid = sch.GetModeId();
		res->printf("{\"mid\":%d,\"md\":[%d,%d],\"count\":%u,", mid, m->on, m->off, count);

		const log_t* data = logs.GetLatest(1);
		if (data != nullptr) {
			res->printf("\"sen\":[%d,%d],", data->temp, data->hum);
		}

		res->printf("\"pin\":%u,", pin.GetOutput());
		res->printf("\"log\":%u,", logs.Count());
		res->printf("\"heap\":%u}", ESP.getFreeHeap());

		req->send(res);
	});

	server.on("/sch/ls", HTTP_GET, [](AsyncWebServerRequest *req){
		AsyncResponseStream *res = req->beginResponseStream("text/json");

		res->printf("{\"def\":[\n");
		for(unsigned i = 0; i < 7; i++){
			const mode* m = sch.GetDef(i);
			res->printf("[%u,%u]%s", m->on, m->off, (i != 6) ? ",\n":"");
		}

		res->printf("],\"sch\":[\n");
		unsigned count = sch.Count();
		for(unsigned i = 0; i < count; i++){
			const schedule* data = sch.Get(i);
			res->printf("[%u,%u,%u,%u,%u]", data->start.week, data->start.sec, data->end.sec, data->m.on, data->m.off);
			if(i < count - 1) res->printf(",\n");
		}
		res->printf("],");

		int16_t mid = sch.GetModeId();
		res->printf("\"mid\":%d,\"count\":%u}\n", mid, count);

		req->send(res);
	});

	server.on("/sch/add", HTTP_GET, [](AsyncWebServerRequest *req){
		PARAM_CHECK("w");
		PARAM_CHECK("as");
		PARAM_CHECK("bs");
		PARAM_CHECK("on");
		PARAM_CHECK("of");

		unsigned w = PARAM_GET_INT("w") - 1;
		unsigned as = PARAM_GET_INT("as") - 1;
		unsigned bs = PARAM_GET_INT("bs") - 1;
		unsigned on = PARAM_GET_INT("on") - 1;
		unsigned of = PARAM_GET_INT("of") - 1;

		int idx = sch.Add(daytime{w, as}, daytime{w, bs}, mode{on, of});
		req->send(200, "text/plain", String(idx));
	});

	server.on("/sch/mod", HTTP_GET, [](AsyncWebServerRequest *req){
		PARAM_CHECK("w");
		PARAM_CHECK("as");
		PARAM_CHECK("bs");
		PARAM_CHECK("on");
		PARAM_CHECK("of");

		unsigned i = PARAM_GET_INT("i") - 1;
		unsigned w = PARAM_GET_INT("w") - 1;
		unsigned as = PARAM_GET_INT("as") - 1;
		unsigned bs = PARAM_GET_INT("bs") - 1;
		unsigned on = PARAM_GET_INT("on") - 1;
		unsigned of = PARAM_GET_INT("of") - 1;

		bool ret = sch.Mod(i, daytime{w, as}, daytime{w, bs}, mode{on, of});
		req->send(200, "text/plain", String(ret));
	});

	server.on("/sch/rm", HTTP_GET, [](AsyncWebServerRequest *req){
		int params = req->params();
		if (params != 1) WERR(req);
		if(!req->hasParam("i")) WERR(req);

		unsigned idx = unsigned(req->getParam("i")->value().toInt());
		req->send(200, "text/plain", String(sch.Del(idx)));
	});

	server.on("/mode", HTTP_GET, [](AsyncWebServerRequest *req){
		PARAM_CHECK("on");
		PARAM_CHECK("of");

		unsigned on = PARAM_GET_INT("on") - 1;
		unsigned of = PARAM_GET_INT("of") - 1;
		pin.SetMode(on, of);

		req->send(200, "text/plain", String(ESP.getFreeHeap()));
	});

	server.on("/log/all", HTTP_GET, [](AsyncWebServerRequest *req){
		uint16_t* vars = new uint16_t[2];
		vars[0] = 0;
		vars[1] = logs.Count();

		AsyncResponseStreamChunked *res = req->beginResponseStreamChunked("text/plain", [vars](AsyncResponseStreamChunked* res, size_t maxLen) {
			size_t ret = 0;
			uint16_t count = vars[1];

			while (ret < maxLen - 16) {
				if(vars[0] < count){
					const log_t* data = logs.Get(vars[0]);
					ret += res->printf("%u,%d,%d\n", vars[0], data->temp, data->hum);
					vars[0] += 1;
				} else {
					Serial.print("count = ");
					Serial.print(count);
					Serial.print(", i = ");
					Serial.print(vars[0]);
					Serial.print(", res = ");
					Serial.println((unsigned int)res, HEX);

					delete vars;
					res->end();
					break;
				}
			}
		});

		res->printf("%u\n", vars[1]);
		req->send(res);
	});

	server.on("/log", HTTP_GET, [](AsyncWebServerRequest *req){
		PARAM_CHECK("c");

		uint16_t* vars = new uint16_t[2];
		vars[0] = 0;
		vars[1] = PARAM_GET_INT("c");

		if(vars[1] > logs.Count()) vars[1] = logs.Count();

		AsyncResponseStreamChunked *res = req->beginResponseStreamChunked("text/plain", [vars](AsyncResponseStreamChunked* res, size_t maxLen) {
			UNUSED(maxLen);
			uint16_t count = vars[1];

			if(vars[0] < count){
				const log_t* data = logs.GetLatest(vars[0]);
				res->printf("%d,%d\n", data->temp, data->hum);
				vars[0] += 1;
			} else {
				delete vars;
				res->end();
			}
		});
		req->send(res);
	});

}

