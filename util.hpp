#ifndef __UTIL_HPP_
#define __UTIL_HPP_

class String2: public String {
    public:
	String str;

	using String::String;

	void SetLength(unsigned int _len) {
		len = _len;
	}
};

String readStringUntil(String& str, char terminator) {
	int index = str.indexOf(terminator);
	if (index < 0) {
		index = str.length();
	}
	String ret = str.substring(0, index);
	str = str.substring(index + 1);
	return ret;
}

// SDK random
extern "C" int os_get_random(unsigned char *buf, size_t len);

#define MAX_RNG_TRY 8
void RNG(uint8_t *dest, unsigned size) {
	os_get_random(dest, size);
}

#endif //__UTIL_HPP_

