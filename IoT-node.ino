/*

*/

#include <ESP8266WiFi.h>
#include <FS.h>

#include <time.h>                       // time() ctime()
#include <coredecls.h>                  // settimeofday_cb()

#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>

#include "Adafruit_BME280.h"

#include "config.h"
#include "setting.hpp"
#include "util.hpp"
#include "schedule.hpp"
#include "sensor_log.hpp"

#include "auth.hpp"
#include "web.hpp"

#define TZ_MN           ((TZ)*60)
#define TZ_SEC          ((TZ)*3600)
#define DST_SEC         ((DST_MN)*60)

bool cbtime_set = false;

void time_is_set (void) {
	cbtime_set = true;
	Serial.println("\n------------------ settimeofday() was called ------------------");
}

Adafruit_BME280 bme;

Log logs;
log_t newest_log;

Scheduler sch;
Mode pin;

Auth auth;

AsyncWebServer server(80);

Settings config;

void setup() {
	Serial.begin(115200);

	bool status = bme.begin(0x76);
	if (!status) {
		Serial.println("Could not find a valid BME280 sensor, check wiring!");
	}

	if (!SPIFFS.begin()) {
		Serial.println("Could not mount SPIFFS!");
	}

	// load config
	config.Load();

	Serial.printf("WiFi Mode = %d\n", config.WiFi_mode);
	Serial.printf("AP_SSID = %s, channel = %d, hidden = %d\n", config.AP_ssid.c_str(), config.AP_chan, config.AP_hidden);
	Serial.printf("STA_SSID = %s\n", config.STA_ssid.c_str());

	configTime(TZ_SEC, DST_SEC, "1.tw.pool.ntp.org", "1.asia.pool.ntp.org", "pool.ntp.org");

	WiFi.persistent(false); // !!! less flash write for WiFi setting !!!
	WiFi.mode(config.WiFi_mode);
	switch (config.WiFi_mode) {
		case WIFI_STA:
			WiFi.begin(config.STA_ssid.c_str(), config.STA_pwd.c_str());
			break;
		case WIFI_AP:
			WiFi.softAP(config.AP_ssid.c_str(), config.AP_pwd.c_str(), config.AP_chan, config.AP_hidden);
			break;
		default:
		case WIFI_AP_STA:
			WiFi.begin(config.STA_ssid.c_str(), config.STA_pwd.c_str());
			WiFi.softAP(config.AP_ssid.c_str(), config.AP_pwd.c_str(), config.AP_chan, config.AP_hidden);
			break;
	}
	auth.setKey((uint8_t*)config.KEY.c_str());


	// TODO: load from SPIFFS
	for(uint8_t i=0; i<7; i++){
		sch.SetDefaultMode(i, i, i+1);
	}
	pin.SetMode(15, 10, true);
	pinMode(OUT_PIN, OUTPUT);


	// don't wait, observe time changing when ntp timestamp is received
	settimeofday_cb(time_is_set);

	setupServer(server);

	// for debug
	server.on("/dump", HTTP_GET, [](AsyncWebServerRequest *req){
		AsyncResponseStream *res = req->beginResponseStream("text/plain");
		unsigned count = sch.Count();
		const mode* m = sch.GetOutput();
		int16_t mid = sch.GetModeId();
		res->printf("sch:%d,%u,%u,%u\n", mid, count, m->on, m->off);

		//res->printf("tsk:%u\n", tsk.GetTaskId());

		const log_t* data = &newest_log;
		res->printf("sen:%d,%d,%d\n", data->temp, data->hum, data->press);

		res->printf("pin:%u\n", pin.GetOutput());
		res->printf("log:%u\n", logs.Count());
		res->printf("heap:%u\n", ESP.getFreeHeap());
		res->printf("url:%s\n", req->url().c_str());
		res->printf("EspId:%x\n", ESP.getChipId());
		res->printf("CpuFreq:%x\n", ESP.getCpuFreqMHz());

		req->send(res);
	});

	const char * buildTime = __DATE__ " " __TIME__ " GMT";
	server.serveStatic("/", SPIFFS, "/web/").setDefaultFile("index.html").setLastModified(buildTime);

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

	if(now_ms - last_ms > 1000){
		last_ms = now_ms;

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
			digitalWrite(OUT_PIN, OUT_LOW_ACTIVE - o);
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
		Serial.println("\n\n");


		int32_t temp = bme.readTemperatureInt();
		int32_t hum = bme.readHumidityInt();
		int32_t press = bme.readPressureInt();
		Serial.printf("Temperature = %f C\n", temp / 100.0f);
		Serial.printf("Humidity = %f %%\n", hum / 1024.0f);

		Serial.printf("Pressure = %f hPa\n\n\n", press / 25600.0f);

		newest_log.temp = (temp << 5) / 100;
		newest_log.hum = (hum >> 8) * 10;
		newest_log.press = (press >> 7) - 101300*2;

		static unsigned log_denom = 0;
		if (log_denom == 0) {
			//logs.Add(newest_log.temp, newest_log.hum, newest_log.press);
			logs.Add(newest_log);
			log_denom = 60;
		}
		log_denom--;
	}

	// 5 times 'R' to reset config
	if (Serial.available() > 0) {
		static uint8_t count = 0;
		int inByte = Serial.read();
		if (inByte == 'R') {
			count++;
			if (count >= 5) {
				Serial.println("reset all config...");
				config.Reset();
				//ESP.restart();
			}
		} else {
			count = 0;
		}
	}
}

void setupServer(AsyncWebServer& server) {

	// get & processing setting
	server.on("/setting", HTTP_GET, [](AsyncWebServerRequest *req){

		Serial.printf("\n\nWiFi Mode = %d\n", config.WiFi_mode);
		Serial.printf("AP_SSID = %s, AP_PWD = %s, channel = %d, hidden = %d\n", config.AP_ssid.c_str(), config.AP_pwd.c_str(), config.AP_chan, config.AP_hidden);
		Serial.printf("STA_SSID = %s, STA_PWD = %s\n\n\n", config.STA_ssid.c_str(), config.STA_pwd.c_str());

		// TODO: encrypt before send
		AsyncResponseStream *res = req->beginResponseStream("text/json");
		res->printf("{\"mode\":%d,", config.WiFi_mode);
		res->printf("\"ap\":\"%s\",", config.AP_ssid.c_str());
		res->printf("\"ch\":%d,", config.AP_chan);
		res->printf("\"hide\":%d,", config.AP_hidden);
		res->printf("\"sta\":\"%s\"}", config.STA_ssid.c_str());

		req->send(res);
	});
	server.on("/setting", HTTP_POST, [](AsyncWebServerRequest *req){
		if(!req->hasParam("c", true)) {
			WRET_PARAM_ERR(req);
			return;
		}
		String c = req->getParam("c", true)->value();
		if (c.length() > MAX_POST_LEN) {
			WRET_PARAM_ERR(req);
			return;
		}

		String buf = auth.CodeHex2Byte((uint8_t*)c.c_str(), c.length());
		auth.SetGenerate(true);

		String magic = readStringUntil(buf, '\n');
		if (!magic.equals(REQ_MAGIC)) {
			WRET_AUTH_ERR(req);
			return;
		}

		int mode = readStringUntil(buf, '\n').toInt();
		String ap_ssid = readStringUntil(buf, '\n');
		String ap_pwd = readStringUntil(buf, '\n');
		int chan = readStringUntil(buf, '\n').toInt();
		int hidden = readStringUntil(buf, '\n').toInt();
		String sta_ssid = readStringUntil(buf, '\n');
		String sta_pwd = readStringUntil(buf, '\n');

		String new_key = readStringUntil(buf, '\n');


		if (mode >= 1 && mode <= 3) config.WiFi_mode = (WiFiMode_t) mode;

		if (ap_ssid.length() > 0 && ap_ssid.length() < 32) config.AP_ssid = ap_ssid;
		if (ap_pwd.length() >= 8 && ap_pwd.length() < 64) config.AP_pwd = ap_pwd;

		if (chan > 0 && chan <= 14) config.AP_chan = chan;

		if (hidden >= 0 && hidden <= 1) config.AP_hidden = hidden;

		if (sta_ssid.length() > 0 && sta_ssid.length() < 32) config.STA_ssid = sta_ssid;
		if (sta_pwd.length() >= 8 && sta_pwd.length() < 64) config.STA_pwd = sta_pwd;

		config.Save();

		if (new_key.length() == 64) {
			config.SetKeyHex(new_key);
			config.SaveKey();
			auth.setKey((uint8_t*)config.KEY.c_str());
		}

		req->send(200, "text/plain", "ok");
	});

	server.on("/token", HTTP_GET, [](AsyncWebServerRequest *req){
		req->send(200, "text/plain", auth.GetIVHex());
	});

	server.on("/status", HTTP_GET, [](AsyncWebServerRequest *req){
		AsyncResponseStream *res = req->beginResponseStream("text/json");
		unsigned count = sch.Count();
		const mode* m = sch.GetOutput();
		int16_t mid = sch.GetModeId();
		res->printf("{\"mid\":%d,\"md\":[%d,%d],\"count\":%u,", mid, m->on, m->off, count);

		const log_t* data = &newest_log;
		res->printf("\"sen\":[%d,%d,%d],", data->temp, data->hum, data->press);


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

		// TODO: check input
		unsigned w = PARAM_GET_INT("w") - 1; // 0~6
		unsigned as = PARAM_GET_INT("as") - 1; // 0 ~ 86399
		unsigned bs = PARAM_GET_INT("bs") - 1; // 0 ~ 86399
		unsigned on = PARAM_GET_INT("on") - 1;
		unsigned of = PARAM_GET_INT("of") - 1;

		if(w >= 7) WERRC(req, 400);
		if(as >= 86400) WERRC(req, 400);
		if(bs >= 86400) WERRC(req, 400);

		bool ok = authCheck(req, auth);
		if (!ok) {
			WRET_AUTH_ERR(req);
			return;
		}

		int idx = sch.Add(daytime{w, as}, daytime{w, bs}, mode{on, of});
		req->send(200, "text/plain", String(idx));
	});

	server.on("/sch/mod", HTTP_GET, [](AsyncWebServerRequest *req){
		PARAM_CHECK("i");
		PARAM_CHECK("w");
		PARAM_CHECK("as");
		PARAM_CHECK("bs");
		PARAM_CHECK("on");
		PARAM_CHECK("of");

		// TODO: check input
		unsigned i = PARAM_GET_INT("i") - 1;
		unsigned w = PARAM_GET_INT("w") - 1; // 0~6
		unsigned as = PARAM_GET_INT("as") - 1; // 0 ~ 86399
		unsigned bs = PARAM_GET_INT("bs") - 1; // 0 ~ 86399
		unsigned on = PARAM_GET_INT("on") - 1;
		unsigned of = PARAM_GET_INT("of") - 1;

		if(w >= 7) WERRC(req, 400);
		if(as >= 86400) WERRC(req, 400);
		if(bs >= 86400) WERRC(req, 400);

		bool ok = authCheck(req, auth);
		if (!ok) {
			WRET_AUTH_ERR(req);
			return;
		}

		bool ret = sch.Mod(i, daytime{w, as}, daytime{w, bs}, mode{on, of});
		req->send(200, "text/plain", String(ret));
	});

	server.on("/sch/rm", HTTP_GET, [](AsyncWebServerRequest *req){
		PARAM_CHECK("i");
		unsigned idx = PARAM_GET_INT("i") - 1;

		bool ok = authCheck(req, auth);
		if (!ok) {
			WRET_AUTH_ERR(req);
			return;
		}

		req->send(200, "text/plain", String(sch.Del(idx)));
	});

	server.on("/sch/def", HTTP_GET, [](AsyncWebServerRequest *req){
		PARAM_CHECK("w");
		PARAM_CHECK("on");
		PARAM_CHECK("of");

		unsigned w = PARAM_GET_INT("w") - 1;
		unsigned on = PARAM_GET_INT("on") - 1;
		unsigned of = PARAM_GET_INT("of") - 1;

		bool ok = authCheck(req, auth);
		if (!ok) {
			WRET_AUTH_ERR(req);
			return;
		}

		if(w <= 7) {
			sch.SetDefaultMode(w, on, of);
		} else {
			sch.SetDefaultMode(on, of);
		}
		req->send(200, "text/plain", "ok");
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
					ret += res->printf("%u,%d,%d,%d\n", vars[0], data->temp, data->hum, data->press);
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
				res->printf("%d,%d,%d\n", data->temp, data->hum, data->press);
				vars[0] += 1;
			} else {
				delete vars;
				res->end();
			}
		});
		req->send(res);
	});

}

