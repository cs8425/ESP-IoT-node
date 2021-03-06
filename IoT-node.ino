/*
ESP-Security-IoT-node

scheduled and settable relay output controller via Web UI, try to maximum security without TLS.

*/

#include <ESP8266WiFi.h>
#include <FS.h>

#include <time.h>                       // time() ctime()

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

Adafruit_BME280 bme;

Log logs;
log_t newest_log;

Scheduler sch;
Mode pin;

Auth auth;

AsyncWebServer server(80);

Settings config;
char chipid_buf[8];

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

	sprintf(chipid_buf, "%08X", ESP.getChipId());

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
	auth.setKey(config.KEY);


	// set default
	sch.SetDefaultMode(DEF_ON_TIME, DEF_OFF_TIME);
	pin.SetMode(DEF_ON_TIME, DEF_OFF_TIME, true);
	pinMode(OUT_PIN, OUTPUT);

	// LED for debug
	pinMode(LED_BUILTIN, OUTPUT);

	// load from SPIFFS
	LoadSchedule(sch);

	setupServer(server);

	// for debug
	server.on("/dump", HTTP_GET, [](AsyncWebServerRequest *req){
		AsyncResponseStream *res = req->beginResponseStream("text/plain", RES_BUF_SIZE);
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
		res->printf("flashId:%x\n", ESP.getFlashChipId());
		res->printf("CpuFreq:%x\n", ESP.getCpuFreqMHz());

		res->end();
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
uint32_t now_ms;
uint32_t next_ms = 0;
uint32_t ext_cmd = 0;

char temp_buf[7];
char hum_buf[7];
char press_buf[8];

void loop() {

	now_ms = millis();
	if (next_ms == 0) next_ms = now_ms;
	if (now_ms - next_ms >= 1000) {
		uint32_t dt = now_ms - next_ms;
		next_ms = now_ms + 1000 - dt;

		now = time(nullptr);


		int sid = sch.Update(now);
		const mode* m = sch.GetOutput();
		if (sid) {
			pin.SetMode(m);
		}

		int o = pin.Update();
		if (o != -1) {
			digitalWrite(OUT_PIN, OUT_LOW_ACTIVE - o);
			digitalWrite(LED_BUILTIN, 1 - o);
		}

		Serial.print("mode.on = ");
		Serial.print(m->on);
		Serial.print(", mode.off = ");
		Serial.println(m->off);
		Serial.print("mode output = ");
		Serial.println(pin.GetOutput());

		Serial.print("log count = ");
		Serial.println(logs.Count());

		Serial.print("wifi status: ");
		Serial.print(WiFi.status());
		Serial.print(", wifi SSID: ");
		Serial.print(WiFi.SSID());
		Serial.print(", IP Address: ");
		Serial.println(WiFi.localIP());

		Serial.print("Free heap: ");
		Serial.println(ESP.getFreeHeap(), DEC);

		printTm("localtime", localtime(&now));
		Serial.print("\n\n\n");


		int32_t temp = bme.readTemperatureInt();
		int32_t hum = bme.readHumidityInt();
		int32_t press = bme.readPressureInt();

		newest_log.temp = (temp << 5) / 100;
		newest_log.hum = (hum * 5) >> 7;
		newest_log.press = (press >> 7) - 101300*2;

		sprintf(temp_buf, "%3.2f\0", newest_log.temp / 32.0);
		sprintf(hum_buf, "%3.2f\0", newest_log.hum / 40.0);
		sprintf(press_buf, "%4.2f\0", (newest_log.press / 200.0) + 1013.0);
		Serial.printf("temp_celsius %s\n", temp_buf);
		Serial.printf("humidity_percent %s\n", hum_buf);
		Serial.printf("pressure_hpa %s\n\n\n", press_buf);

		static unsigned log_denom = 0;
		if (log_denom == 0) {
			//logs.Add(newest_log.temp, newest_log.hum, newest_log.press);
			logs.Add(newest_log);
			log_denom = LOG_DENOM;

			/*sprintf(temp_buf, "%3.2f", newest_log.temp / 32.0);
			sprintf(hum_buf, "%3.2f", newest_log.hum / 40.0);
			sprintf(press_buf, "%4.2f", (newest_log.press / 200.0) + 1013.0);*/
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

				SPIFFS.remove(SCHEDLE_FILE);
			}
		} else {
			count = 0;
		}
	}

	if (ext_cmd) {
		switch (ext_cmd) {
		case 1: // reboot
			Serial.println("reboot...");
			ESP.restart();
			break;
		case 2: // flush
			Serial.println("flush...");
			SaveSchedule(sch);
			break;
		case 3: // load
			Serial.println("load...");
			LoadSchedule(sch);
			break;
		}
		ext_cmd = 0;
	}

	// 0 ~ 255
	delay(config.PWR_SLEEP); // https://github.com/arendst/Sonoff-Tasmota/wiki/Energy-Saving
}

void setupServer(AsyncWebServer& server) {

	// get & processing setting
	server.on("/setting", HTTP_GET, [](AsyncWebServerRequest *req){

		// TODO: encrypt before send
		AsyncResponseStream *res = req->beginResponseStream("text/json", RES_BUF_SIZE);
		res->printf("{\"mode\":%d,", config.WiFi_mode);
		res->printf("\"ap\":\"%s\",", config.AP_ssid.c_str());
		res->printf("\"ch\":%d,", config.AP_chan);
		res->printf("\"hide\":%d,", config.AP_hidden);
		res->printf("\"sta\":\"%s\",", config.STA_ssid.c_str());
		res->printf("\"tag\":\"%s\",", config.Tag.c_str());
		res->printf("\"pwr\":%d}", config.PWR_SLEEP);

		res->end();
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

		int sleep = readStringUntil(buf, '\n').toInt();

		String new_key = readStringUntil(buf, '\n');

		String new_tag = readStringUntil(buf, '\n');


		if (mode >= 1 && mode <= 3) config.WiFi_mode = (WiFiMode_t) mode;

		if (ap_ssid.length() > 0 && ap_ssid.length() < 32) {
			config.AP_ssid = ap_ssid;
			if (ap_pwd.length() >= 8 && ap_pwd.length() < 64) config.AP_pwd = ap_pwd;
		}

		if (chan > 0 && chan <= 14) config.AP_chan = chan;

		if (hidden >= 0 && hidden <= 1) config.AP_hidden = hidden;

		if (sta_ssid.length() > 0 && sta_ssid.length() < 32) {
			config.STA_ssid = sta_ssid;
			if (sta_pwd.length() >= 8 && sta_pwd.length() < 64) config.STA_pwd = sta_pwd;
		}

		if (sleep >= 0 && sleep <= 250) config.PWR_SLEEP = sleep;

		if (new_tag.length() > 0 && new_tag.length() <= 20) {
			config.Tag = new_tag;
		}

		config.Save();

		if (new_key.length() == 64) {
			config.SetKeyHex(new_key);
			config.SaveKey();
			auth.setKey(config.KEY);
		}

		req->send(200, "text/plain", "ok");
	});

	server.on("/sys", HTTP_GET, [](AsyncWebServerRequest *req){
		PARAM_CHECK("c");
		unsigned c = PARAM_GET_INT("c");
		// 0 : nop
		// 1 : reboot
		// 2 : flush schedule data to SPIFFS
		// 3 : load schedule data from SPIFFS

		bool ok = authCheck(req, auth);
		if (!ok) {
			WRET_AUTH_ERR(req);
			return;
		}

		ext_cmd = c; // set event, and do in loop()

		req->send(200, "text/plain", String(c));
	});

	server.on("/token", HTTP_GET, [](AsyncWebServerRequest *req){
		req->send(200, "text/plain", auth.GetIVHex());
	});

	server.on("/status", HTTP_GET, [](AsyncWebServerRequest *req){
		AsyncResponseStream *res = req->beginResponseStream("text/json", RES_BUF_SIZE);
		unsigned count = sch.Count();
		const mode* m = sch.GetOutput();
		int16_t mid = sch.GetModeId();
		res->printf("{\"mid\":%d,\"md\":[%d,%d],\"count\":%u,", mid, m->on, m->off, count);

		const log_t* data = &newest_log;
		res->printf("\"sen\":[%d,%d,%d],", data->temp, data->hum, data->press);


		res->printf("\"pin\":%u,", pin.GetOutput());
		res->printf("\"log\":%u,", logs.Count());
		res->printf("\"heap\":%u}", ESP.getFreeHeap());

		res->end();
		req->send(res);
	});

	server.on("/sch/ls", HTTP_GET, [](AsyncWebServerRequest *req){
		AsyncResponseStream *res = req->beginResponseStream("text/json", RES_BUF_SIZE);

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

		res->end();
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

		if(w < 7) {
			sch.SetDefaultMode(w, on, of);
		} else {
			sch.SetDefaultMode(on, of);
		}
		req->send(200, "text/plain", "ok");
	});

	server.on("/log/all", HTTP_GET, [](AsyncWebServerRequest *req){
		uint16_t* vars = new(std::nothrow) uint16_t[2];
		if (vars == nullptr) {
			req->send(500);
			return;
		}

		vars[0] = 0;
		vars[1] = logs.Count();

		req->onDisconnect([vars](){
			delete vars; // free up allocated memory
		});

		AsyncResponseStream *res = req->beginResponseStreamChunked("text/plain", [vars](AsyncResponseStream* res, size_t maxLen) {
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

					res->end();
					break;
				}
			}
		}, RES_BUF_SIZE);

		res->printf("%u\n", vars[1]);
		req->send(res);
	});

	server.on("/log", HTTP_GET, [](AsyncWebServerRequest *req){
		PARAM_CHECK("c");

		uint16_t* vars = new(std::nothrow) uint16_t[2];
		if (vars == nullptr) {
			req->send(500);
			return;
		}

		vars[0] = 0;
		vars[1] = PARAM_GET_INT("c");

		req->onDisconnect([vars](){
			delete vars; // free up allocated memory
		});

		if(vars[1] > logs.Count()) vars[1] = logs.Count();

		AsyncResponseStream *res = req->beginResponseStreamChunked("text/plain", [vars](AsyncResponseStream* res, size_t maxLen) {
			UNUSED(maxLen);
			uint16_t count = vars[1];

			if(vars[0] < count){
				const log_t* data = logs.GetLatest(vars[0]);
				res->printf("%d,%d,%d\n", data->temp, data->hum, data->press);
				vars[0] += 1;
			} else {
				res->end();
			}
		}, RES_BUF_SIZE);
		req->send(res);
	});

	server.on("/metrics", HTTP_GET, [](AsyncWebServerRequest *req){
		AsyncResponseStream *res = req->beginResponseStream("text/plain; version=0.0.4", RES_BUF_SIZE);
		res->addHeader("Server","ESP8266 IoT Server");

		res->printf("# HELP temp_celsius The temperature of the node in Celsius.\n# TYPE temp_celsius gauge\n");
		res->printf("temp_celsius{id=\"%s\",tag=\"%s\"} %s\n\n", chipid_buf, config.Tag.c_str(), temp_buf);

		res->printf("# HELP humidity_percent The pressure of the node in %%.\n# TYPE humidity_percent gauge\n");
		res->printf("humidity_percent{id=\"%s\",tag=\"%s\"} %s\n\n", chipid_buf, config.Tag.c_str(), hum_buf);

		res->printf("# HELP pressure_hpa The pressure of the node in hPa.\n# TYPE pressure_hpa gauge\n");
		res->printf("pressure_hpa{id=\"%s\",tag=\"%s\"} %s\n\n", chipid_buf, config.Tag.c_str(), press_buf);

		res->printf("# TYPE uptime_seconds counter\nuptime_seconds{id=\"%s\",tag=\"%s\"} %d\n\n", chipid_buf, config.Tag.c_str(), millis()/1000);
		res->printf("# TYPE heap_bytes gauge\nheap_bytes{id=\"%s\",tag=\"%s\"} %d\n\n", chipid_buf, config.Tag.c_str(), ESP.getFreeHeap());

		req->send(res);
	});

}

void LoadSchedule(Scheduler& sch) {
	if (!SPIFFS.exists(SCHEDLE_FILE)) return;
	File f = SPIFFS.open(SCHEDLE_FILE, "r");
	if (!f) return;

	for (unsigned i=0; i<7; i++) {
		mode m;
		f.readBytes((char*) &m, sizeof(m));
		sch.SetDefaultMode(i, m.on, m.off);
	}

	sch.Clear();
	schedule d;
	while(f.readBytes((char*) &d, sizeof(d))) {
		int ret = sch.Add(d.start, d.end, d.m);
		if (ret == -1) break;
	}

	f.close();
}

void SaveSchedule(Scheduler& sch) {
	File f = SPIFFS.open(SCHEDLE_FILE, "w+");
	if (!f) return;

	for(unsigned i=0; i<7; i++){
		const mode* m = sch.GetDef(i);
		f.write((uint8_t*) m, sizeof(mode));
	}

	unsigned count = sch.Count();
	for(unsigned i=0; i<count; i++){
		const schedule* data = sch.Get(i);
		f.write((uint8_t*) data, sizeof(schedule));
	}

	f.close();
}

