#ifndef __AUTH_HPP_
#define __AUTH_HPP_

#define CBC 0
#define CTR 1
#define ECB 0

#define AES256 1

#include "aes.hpp"

extern void RNG(uint8_t *dest, unsigned size);
//extern uint32_t millis(void);
//extern unsigned long millis(void);

#define IV_SIZE 16
#define KEY_SIZE 32
#define IV_TIMEOUT 10000
class Auth {
	public:
		Auth() {
			ivHex[IV_SIZE*2] = 0;
			_needgen = true;
		}

		void setKey(const uint8_t* key) {
			memcpy(_key, key, KEY_SIZE);
			_needgen = true;
		}

		const char* GetIVHex() {
			uint32_t now = millis();
			if (needGenerate(now)) {
				GenerateIV();
				_lastgen = now;
			}
			return ivHex;
		}

		bool CheckKey(uint8_t* sign, size_t len, uint8_t* input) {
			if (_needgen) return false;
			_needgen = true;

			AES_CTR_xcrypt_buffer(&_ctx, sign, len);

			if (0 == memcmp((char *) sign, (char *) input, len)) {
				return true;
			}
			return false;
		}

		bool CheckKeyHex(uint8_t* signHex, size_t lenHex, uint8_t* input) {
			if (lenHex & 0x01) return false;
			size_t len = lenHex / 2;
			uint8_t* sign = new uint8_t[len];

			unsigned i,j;
			for (i=0, j=0; i<len; i++, j+=2) {
				uint8_t c = hex2byte(signHex[j]) << 4;
				sign[i] = c | hex2byte(signHex[j + 1]);
			}

			bool ok = CheckKey(sign, len, input);
			delete sign;
			return ok;
		}

		void Sign(uint8_t* sign, size_t len) { // encode / decode
			AES_CTR_xcrypt_buffer(&_ctx, sign, len);
		}

		String CodeHex2Byte(uint8_t* signHex, size_t lenHex) {
			String sign = String();
			if (lenHex & 0x01) return String(sign);
			size_t len = lenHex / 2;
			sign.reserve(len);

			unsigned i,j;
			for (i=0, j=0; i<len; i++, j+=2) {
				uint8_t c = hex2byte(signHex[j]) << 4;
				//sign[i] = c | hex2byte(signHex[j + 1]);
				//sign.setCharAt(i, c | hex2byte(signHex[j + 1]));
				sign += (char) (c | hex2byte(signHex[j + 1]));
			}

			//Serial.printf("sign0(%d, %d) = %s\n", len, sign.length(), sign.c_str());
			Sign((uint8_t*)sign.begin(), len);
			//Serial.printf("sign1(%d) = %s\n", sign.length(), sign.c_str());
			return String(sign);
		}

		void Reset() {
			_needgen = false;
			AES_init_ctx_iv(&_ctx, _key, _iv);
		}

		void SetGenerate(bool need) {
			_needgen = need;
		}

		bool needGenerate(uint32_t now) {
			return _needgen || (now - _lastgen > IV_TIMEOUT);
		}

		void GenerateIV() {
			RNG(_iv, IV_SIZE);
			unsigned i,j;
			for (i=0,j=0; i<IV_SIZE; i++, j+=2) {
				ivHex[j] = HEXMAP[ (_iv[i] >> 4) & 0xf ];
				ivHex[j+1] = HEXMAP[ _iv[i] & 0xf ];
			}
			_needgen = false;
			AES_init_ctx_iv(&_ctx, _key, _iv);
		}

	private:
		static uint8_t hex2byte(uint8_t c) {
			return c - ( c <= '9' ? '0' : ('a'-10) );
		}

		static const char* HEXMAP;
		uint8_t _iv[IV_SIZE];
		char ivHex[IV_SIZE*2 + 1];

		uint8_t _key[KEY_SIZE];

		struct AES_ctx _ctx;

		bool _needgen;
		uint32_t _lastgen;
};

const char *Auth::HEXMAP = "0123456789abcdef";

#endif //__AUTH_HPP_

