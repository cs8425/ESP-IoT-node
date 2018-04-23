
/*
#ifdef ESP8266
extern "C" {
#include "user_interface.h"
}
#endif
*/

// SDK get time from boot up
extern "C" int clock_gettime(clockid_t unused, struct timespec *tp);

// SDK random
extern "C" unsigned long os_random(void);
extern "C" int os_get_random(unsigned char *buf, size_t len);


// CRC function used to ensure data validity
uint32_t calculateCRC32(const uint8_t *data, size_t length);

// Structure which will be stored in RTC memory.
// First field is CRC32, which is calculated based on the
// rest of structure contents.
// Any fields can go after CRC32.
// We use byte array as an example.

struct rtcbackup {
	timeval tv;
	timezone tz;

	uint64_t time_base;
	uint32_t time_acc;
};

struct {
	uint32_t crc32;
//	byte data[508];
	union data {
		rtcbackup d;
		byte b[sizeof(rtcbackup)];
	};
} rtcData;

/*
struct {
	uint32_t crc32;
	byte data[508];
} rtcData;
*/

uint32_t calculateCRC32(const uint8_t *data, size_t length) {
	uint32_t crc = 0xffffffff;
	while (length--) {
		uint8_t c = *data++;
		for (uint32_t i = 0x80; i > 0; i >>= 1) {
			bool bit = crc & 0x80000000;
			if (c & i) {
				bit = !bit;
			}
			crc <<= 1;
			if (bit) {
				crc ^= 0x04c11db7;
			}
		}
	}
	return crc;
}

/*
	// Read struct from RTC memory
	if (ESP.rtcUserMemoryRead(0, (uint32_t*) &rtcData, sizeof(rtcData))) {
		Serial.println("Read: ");
		printMemory();
		Serial.println();
		//uint32_t crcOfData = calculateCRC32( (uint8_t*) &rtcData.data[0], sizeof(rtcData.data) );
		uint32_t crcOfData = calculateCRC32( (uint8_t*) &rtcData.data.b, sizeof(rtcData.data) );
		Serial.print("CRC32 of data: ");
		Serial.println(crcOfData, HEX);
		Serial.print("CRC32 read from RTC: ");
		Serial.println(rtcData.crc32, HEX);
		if (crcOfData != rtcData.crc32) {
			Serial.println("CRC32 in RTC memory doesn't match CRC32 of data. Data is probably invalid!");
		} else {
			Serial.println("CRC32 check ok, data is probably valid.");
		}
	}

	// Generate new data set for the struct
	for (size_t i = 0; i < sizeof(rtcData.data); i++) {
		rtcData.data[i] = random(0, 128);
	}

	// Update CRC32 of data
	//rtcData.crc32 = calculateCRC32( (uint8_t*) &rtcData.data[0], sizeof(rtcData.data) );
	rtcData.crc32 = calculateCRC32( (uint8_t*) &rtcData.data.b, sizeof(rtcData.data) );
	// Write struct to RTC memory
	if (ESP.rtcUserMemoryWrite(0, (uint32_t*) &rtcData, sizeof(rtcData))) {
		Serial.println("Write: ");
		printMemory();
		Serial.println();
	}
*/
/*

	if(need_init){
		rtc_time.time_acc = 0;
		rtc_time.time_base = system_get_rtc_time();
	}


//	uint32_t st1 = system_get_time();
	uint32_t cal = system_rtc_clock_cali_proc();
	uint32_t rtc_t2 = system_get_rtc_time();

	rtc_time.time_acc += ((uint64_t)(rtc_t2 - rtc_time.time_base)) * ((uint64_t)( (cal*1000) >> 12 ) );
	rtc_time.time_base = rtc_t2;

	// do RTC write back

*/

/*

void initEEPROM() {
	EEPROM.begin(512);
}

void storeStruct(void *data_source, size_t size) {
	for(size_t i = 0; i < size; i++) {
		char data = ((char *)data_source)[i];
		EEPROM.write(i, data);
	}
	EEPROM.commit();
}

void loadStruct(void *data_dest, size_t size) {
	for(size_t i = 0; i < size; i++) {
		char data = EEPROM.read(i);
		((char *)data_dest)[i] = data;
	}
}

typedef struct {
	char board_name[64];
	char ssid[64];
	char pass[64];
	char key[64];
} settings_t __attribute__ ((packed));

settings_t settings {
"test_host",
"ssid",
"pass",
"aes-key",
};

loadStruct(&settings, sizeof(settings));
storeStruct(&settings, sizeof(settings));

EEPROM.get(0, settings);
EEPROM.put(0, settings);

*/

