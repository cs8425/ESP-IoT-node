#ifndef __SETTING_HPP_
#define __SETTING_HPP_

#include <FS.h>

#include "config.h"

class Settings {
	public:
		static String AP_ssid;
		static String AP_pwd;
		static int AP_chan;
		static bool AP_hidden;

		static String STA_ssid;
		static String STA_pwd;

		static WiFiMode_t WiFi_mode;

		static String KEY;

		Settings() {

		}

		int Load() {
			int ret = LoadConfig();
			ret += LoadKey();

			return ret;
		}

		int LoadConfig() {
			if (!SPIFFS.exists(CONFIG_FILE)) return 0;
			File f = SPIFFS.open(CONFIG_FILE, "r");
			if (!f) return 1;

			return Parse(&f);
		}

		int LoadKey() {
			if (!SPIFFS.exists(KEY_FILE)) return 0;
			File f = SPIFFS.open(KEY_FILE, "r");
			if (!f) return 2;

			String key = f.readStringUntil('\n');
			if (key.length() == 32) {
				KEY = key;
			}

			return 0;
		}

		int Parse(File* f) {
			String ap_ssid = f->readStringUntil('\n');
			String ap_pwd = f->readStringUntil('\n');
			int chan = f->readStringUntil('\n').toInt();
			int hidden = f->readStringUntil('\n').toInt();
			String sta_ssid = f->readStringUntil('\n');
			String sta_pwd = f->readStringUntil('\n');
			int mode = f->readStringUntil('\n').toInt();

			if (ap_ssid.length() > 0 && ap_ssid.length() < 32) AP_ssid = ap_ssid;
			if (ap_pwd.length() >= 8 && ap_pwd.length() < 64) AP_pwd = ap_pwd;

			if (sta_ssid.length() > 0 && sta_ssid.length() < 32) STA_ssid = sta_ssid;
			if (sta_pwd.length() >= 8 && sta_pwd.length() < 64) STA_pwd = sta_pwd;
			/*AP_ssid = ap_ssid.c_str();
			AP_pwd = ap_pwd.c_str();
			STA_ssid = sta_ssid.c_str();
			STA_pwd = sta_pwd.c_str();*/

			if ((chan > 0) && (chan <= 14) ){
				AP_chan = chan;
			}

			AP_hidden = (bool) hidden;

			if ((mode >= 1) && (mode <= 3) ){
				WiFi_mode = (WiFiMode_t) mode;
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

};

String Settings::AP_ssid = AP_SSID;
String Settings::AP_pwd = AP_PWD;
int Settings::AP_chan = AP_CHANNEL;
bool Settings::AP_hidden = AP_HIDDEN;

String Settings::STA_ssid = STA_SSID;
String Settings::STA_pwd = STA_PWD;

WiFiMode_t Settings::WiFi_mode = WIFI_MODE;

String Settings::KEY = AES_KEY;

#endif //__SETTING_HPP_

