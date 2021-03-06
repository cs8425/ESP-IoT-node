#include "sensor_log.hpp"

Log::Log() {
	head = 0;
	tail = 0;
}

void Log::next() {
	uint16_t i = (uint16_t)(head + 1);

	if(i == MAX_LOG_COUNT) i = 0;

	// overwrite oldest
	if (i == tail) {
		tail = (uint16_t)(tail + 1);
		if(tail == MAX_LOG_COUNT) tail = 0;
	}

	head = i;
}

/*void Log::Add(uint16_t temp, uint16_t hum, uint16_t press) {
	_log[head].temp = temp;
	_log[head].hum = hum;
	_log[head].press = press;
	next();
}*/

void Log::Add(log_t log) {
	_log[head] = log;
	next();
}

uint16_t Log::Count() {
	if(head == tail) return 0;

	if(head > tail){
		return head - tail;
	}else{
		return head - tail + MAX_LOG_COUNT;
	}
}

log_t* Log::Get(uint16_t idx) {
	if(idx >= Count()) {
		return nullptr;
	}

	uint16_t id = tail + idx;
	if(id >= MAX_LOG_COUNT) {
		id -= MAX_LOG_COUNT;
	}

	return &_log[id];

}

log_t* Log::GetLatest(uint16_t idx) {
	if(idx >= Count()) {
		return nullptr;
	}

	uint16_t id = head - idx + MAX_LOG_COUNT - 1;
	if(id >= MAX_LOG_COUNT) {
		id -= MAX_LOG_COUNT;
	}

	return &_log[id];

}

