#ifndef __SETTING_HPP_
#define __SETTING_HPP_

#include <FS.h>

#include "config.h"
#include "util.hpp"

class Settings {
	public:
		static String AP_ssid;
		static String AP_pwd;
		static bool AP_hidden;
		static int8_t AP_chan;
		static int8_t PWR_SLEEP;

		static String STA_ssid;
		static String STA_pwd;

		static WiFiMode_t WiFi_mode;

		static String KEY;

		Settings() {
			SetKeyHex((uint8_t*)AES_KEY, sizeof(AES_KEY));
		}

		void SetKeyHex(String hex) {
			SetKeyHex((uint8_t*)hex.c_str(), hex.length());
		}
		void SetKeyHex(uint8_t* keyHex, size_t lenHex) {
			String2 key = String2();
			size_t len = lenHex / 2;
			key.reserve(len);
			key.SetLength(len);

			unsigned i,j;
			for (i=0, j=0; i<len; i++, j+=2) {
				uint8_t c = hex2byte(keyHex[j]) << 4;
				key.setCharAt(i, c | hex2byte(keyHex[j + 1]));
			}

			KEY = key;
		}

		int Load() {
			int ret = LoadConfig();
			ret += LoadKey();

			return ret;
		}

		int LoadConfig() {
			if (!SPIFFS.exists(CONFIG_FILE)) return 1;
			File f = SPIFFS.open(CONFIG_FILE, "r");
			if (!f) return 1;

			return Parse(&f);
		}

		int Parse(File* f) {
			String ap_ssid = f->readStringUntil('\n');
			String ap_pwd = f->readStringUntil('\n');
			int chan = f->readStringUntil('\n').toInt();
			int hidden = f->readStringUntil('\n').toInt();
			String sta_ssid = f->readStringUntil('\n');
			String sta_pwd = f->readStringUntil('\n');
			int mode = f->readStringUntil('\n').toInt();
			int sleep = f->readStringUntil('\n').toInt();

			if (ap_ssid.length() > 0 && ap_ssid.length() < 32) {
				AP_ssid = ap_ssid;
				if (ap_pwd.length() >= 8 && ap_pwd.length() < 64) AP_pwd = ap_pwd;
			}

			if (sta_ssid.length() > 0 && sta_ssid.length() < 32) {
				STA_ssid = sta_ssid;
				if (sta_pwd.length() >= 8 && sta_pwd.length() < 64) STA_pwd = sta_pwd;
			}

			if ((chan > 0) && (chan <= 14) ){
				AP_chan = chan;
			}

			AP_hidden = (bool) hidden;

			if ((mode >= 1) && (mode <= 3) ){
				WiFi_mode = (WiFiMode_t) mode;
			}

			if ((sleep >= 0) && (sleep <= 250) ){
				PWR_SLEEP = sleep;
			}

			f->close();
			return 0;
		}

		int Save() {
			File f = SPIFFS.open(CONFIG_FILE, "w+");
			if (!f) return 1;

			f.printf("%s\n", AP_ssid.c_str());
			f.printf("%s\n", AP_pwd.c_str());
			f.printf("%d\n", AP_chan);
			f.printf("%d\n", AP_hidden);
			f.printf("%s\n", STA_ssid.c_str());
			f.printf("%s\n", STA_pwd.c_str());
			f.printf("%d\n", WiFi_mode);

			f.printf("%d\n", PWR_SLEEP);
			f.close();
			return 0;
		}

		int LoadKey() {
			if (!SPIFFS.exists(KEY_FILE)) return 1;
			File f = SPIFFS.open(KEY_FILE, "r");
			if (!f) return 2;

			String key = f.readStringUntil('\n');
			if (key.length() == 32) {
				KEY = key;
			}

			f.close();
			return 0;
		}

		int SaveKey() {
			File f = SPIFFS.open(KEY_FILE, "w+");
			if (!f) return 1;

			f.printf("%s\n", KEY.c_str());
			f.close();
			return 0;
		}

		void Reset() {
			SPIFFS.remove(CONFIG_FILE);
			SPIFFS.remove(KEY_FILE);
		}
	private:
		static uint8_t hex2byte(uint8_t c) {
			return c - ( c <= '9' ? '0' : ('a'-10) );
		}
};

String Settings::AP_ssid = AP_SSID;
String Settings::AP_pwd = AP_PWD;
int8_t Settings::AP_chan = AP_CHANNEL;
bool Settings::AP_hidden = AP_HIDDEN;

String Settings::STA_ssid = STA_SSID;
String Settings::STA_pwd = STA_PWD;

WiFiMode_t Settings::WiFi_mode = WIFI_MODE;

int8_t Settings::PWR_SLEEP = PWR_SLEEP_MS;

String Settings::KEY = "";

#endif //__SETTING_HPP_

