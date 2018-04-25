#ifndef __SENSOT_LOG_HPP_
#define __SENSOT_LOG_HPP_

#include <stdint.h>

// TODO: fix define MAX_LOG_COUNT not work for preallocated array
#ifndef MAX_LOG_COUNT
//#define MAX_LOG_COUNT (60*24*3)
#define MAX_LOG_COUNT (60*36)
#endif

struct log_t {
	// temperature
	unsigned int temp : 12;

	// humidity
	unsigned int hum : 12;
} __attribute__((packed));

class Log {
	public:
		Log();

		void Add(uint16_t temp, uint16_t hum);
		log_t* Get(uint16_t idx); // 0 >> oldest
		log_t* GetLatest(uint16_t idx); // 0 >> latest
		uint16_t Count();

	private:
		uint16_t head;
		uint16_t tail;

		uint32_t _lasttime;
		log_t _log[MAX_LOG_COUNT];

};

#endif //__SENSOT_LOG_HPP_

