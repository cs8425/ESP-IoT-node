#ifndef __UTIL_HPP_
#define __UTIL_HPP_


String readStringUntil(String& str, char terminator) {
	int index = str.indexOf(terminator);
	if (index < 0) {
		index = str.length();
	}
	String ret = str.substring(0, index);
	str = str.substring(index + 1);
	return ret;
}


// SDK get time from boot up
extern "C" int clock_gettime(clockid_t unused, struct timespec *tp);

// SDK random
extern "C" unsigned long os_random(void);
extern "C" int os_get_random(unsigned char *buf, size_t len);

#define MAX_RNG_TRY 8
void RNG(uint8_t *dest, unsigned size) {
	// Use the least-significant bits from the ADC for an unconnected pin (or connected to a source of 
	// random noise). This can take a long time to generate random data if the result of analogRead(0) 
	// doesn't change very frequently.
	while (size) {
		uint8_t val = 0;
		for (unsigned i = 0; i < 8; ++i) {
			int init = analogRead(A0);
			int count = 0;
			while ((analogRead(A0) == init) && (count < MAX_RNG_TRY)) {
				++count;
			}

			if ((count == 0) || (count == MAX_RNG_TRY)){
				val = (val << 1) | (init & 0x01);
			} else {
				val = (val << 1) | (count & 0x01);
			}
		}
		*dest = val;
		++dest;
		--size;
	}
}




// CRC function used to ensure data validity
uint32_t calculateCRC32(const uint8_t *data, size_t length);

/*struct rtcbackup {
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
} rtcData;*/

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

#endif //__UTIL_HPP_

