#ifndef __WEB_HPP_
#define __WEB_HPP_

#include "auth.hpp"
#include "lonesha256.h"

// we won't have too long data to check
#define MAX_PARAM_LEN 256
#define MAX_POST_LEN 512


// TODO: dynamic Magic
#define REQ_MAGIC "ESP23333"

#define WERR(req) { \
	(req)->send(500); \
	return; \
}

#define WERRC(req, c) { \
	(req)->send((c)); \
	return; \
}

const char auth_err[] PROGMEM = "auth error, not-authorized";
const char param_err[] PROGMEM = "param error, missing or too long";

#define WRET_PARAM_ERR(req) { \
	(req)->send_P(400, "text/plain", param_err); \
}

#define WRET_AUTH_ERR(req) { \
	(req)->send_P(403, "text/plain", auth_err); \
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

	if (sign.length() != SHA256SUM_LEN*2) { // length of sha256 hex
		return false;
	}

	String param;
	param.reserve(sign.length()/2);

	int params = req->params() - 1;
	for (int i=0; i<params; i++) {
		AsyncWebParameter* p = req->getParam(i);

		if (p->name() == "k") {
			params += 1;
			continue;
		}

		param += p->name() + "=" + p->value();
		if (i != params-1) param += "&";
	}

	uint8_t out[SHA256SUM_LEN];
	lonesha256(out, (uint8_t*)param.c_str(), param.length());

	bool ok = auth.CheckKeyHex((uint8_t*)sign.c_str(), sign.length(), out);
	return ok;
}

#endif //__WEB_HPP_

