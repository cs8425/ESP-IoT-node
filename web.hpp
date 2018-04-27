#ifndef __WEB_HPP_
#define __WEB_HPP_

#define WERR(req) { \
	(req)->send(500); \
	return; \
}

#define WERRC(req, c) { \
	(req)->send((c)); \
	return; \
}


// return code 400 when Param not set.
#define PARAM_CHECK(x) { \
	if(!req->hasParam( (x) )) { \
		req->send(400); \
		return; \
	} \
}

#define PARAM_CHECKC(x, c) { \
	if(!req->hasParam( (x) )) WERRC(req, (c)); \
}


// .toInt() return 0 when error, so data start with 1
#define PARAM_GET_INT(x) ({\
	int __tmp = req->getParam( (x) )->value().toInt(); \
	if (__tmp == 0) WERR(req); \
	__tmp; \
})


#define PARAM_GET_STR(x, def) ( (req->hasParam( (x) )) ? req->getParam( (x) )->value() : (def) )

bool authCheck(AsyncWebServerRequest *req, String& val) {
	if (!req->hasParam("k")) {
		return false;
	}

	String key = req->getParam("k")->value();
	if (key != val) {
		return false;
	}
	return true;
}

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

//#define IV_SIZE 32
const char* HEXMAP = "0123456789abcdef";
const char* getKey() {
	static uint8_t keyByte[IV_SIZE] = {0};
	static char keyHex[IV_SIZE*2 + 1] = {0};

	RNG(keyByte, IV_SIZE);

	unsigned i,j;
	for (i=0,j=0; i<IV_SIZE; i++, j+=2) {
		keyHex[j] = HEXMAP[ (keyByte[i] >> 4) & 0xf ];
		keyHex[j+1] = HEXMAP[ keyByte[i] & 0xf ];
	}

	return keyHex;
}

#endif //__WEB_HPP_

