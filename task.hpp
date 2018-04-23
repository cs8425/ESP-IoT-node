#ifndef __TASK_HPP_
#define __TASK_HPP_

#include <stdint.h>

#include "mode.hpp"

#define MAX_TASK_COUNT 16

struct task_t {
	// mode
	mode m;

	// delay
	unsigned int delay : 19; // ~145 hr, 0 >> exec immediate

	// en
	unsigned int en : 1;

	// keep
	unsigned int keep : 19; // ~145 hr, 0 >> forever

	// internal flag
	unsigned int rm : 1;

} __attribute__((packed));

class Task {
	public:
		Task();

		int Add(uint32_t delay = 0, uint32_t keep = 0, uint16_t on = 300, uint16_t off = 1500);
		int Add(uint32_t delay, uint32_t keep, mode m);
		bool Del(uint8_t idx);
		const task_t* Get(uint8_t idx);
		uint8_t Count();
		void Clear();

		int Update(); // return 1 >> change, 0 >> don't change
		const mode* GetOutput();

		const task_t* GetD(uint8_t idx);
	private:
		void defragment(bool full = true, uint8_t idx = 0);
		uint8_t _count;
		task_t _tasks[MAX_TASK_COUNT];

		int16_t _lastId;
		mode _output;

};

#endif //__SCHEDLE_HPP_
