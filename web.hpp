#ifndef __WEB_HPP_
#define __WEB_HPP_

#include "auth.hpp"

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

bool authCheck(AsyncWebServerRequest *req, Auth auth) {
	if (!req->hasParam("k")) {
		return false;
	}
	String sign = req->getParam("k")->value();

	String param;
	param.reserve(sign.length());

	int params = req->params();
	for(int i=0; i<params; i++){
		AsyncWebParameter* p = req->getParam(i);
		param += p->name() + "=" + p->value();
		if (i != params-1) param += "&";
	}
	Serial.printf("param[%d]:%s\n", param.length(), param.c_str());
	Serial.printf("sign0[%d]:%s\n", sign.length(), sign.c_str());

	bool ok = auth.CheckKeyHex((uint8_t*)sign.c_str(), sign.length(), (uint8_t*)param.c_str());

	Serial.printf("sign[%d]:%s\n", sign.length(), sign.c_str());

	return ok;
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

#endif //__WEB_HPP_

