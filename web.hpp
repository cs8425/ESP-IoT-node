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
	if(!req->hasParam("key")) {
		req->send(403);
		return false;
	}

	String key = req->getParam("key")->value();
	if (key != val) {
		return false;
	}
	return true;
}


#endif //__WEB_HPP_

