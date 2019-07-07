#ifndef __SCHEDLE_HPP_
#define __SCHEDLE_HPP_

#include <stdint.h>
#include <time.h>

#include "config.h"

#include "mode.hpp"

//#define MAX_SCHEDLE_COUNT 168
#ifndef MAX_SCHEDLE_COUNT
#define MAX_SCHEDLE_COUNT 168
#endif

struct daytime {
	// week 0~6
	unsigned int week : 3;

	// seconds of day
	unsigned int sec : 17;

} __attribute__((packed));

struct schedule {
	// start
	daytime start;

	// end
	daytime end;

	// mode
	mode m;
};

class Scheduler {
	public:
		Scheduler();

		int Add(daytime a, daytime b, uint16_t on, uint16_t off); // start with 0, -1 >> error
		int Add(daytime a, daytime b, mode m); // start with 0, -1 >> error
		bool Mod(uint16_t idx, daytime a, daytime b, mode m); // start with 0, -1 >> error
		bool Del(uint16_t idx);
		const schedule* Get(uint16_t idx);
		uint16_t Count();
		void Clear();

		void SetDefaultMode(uint16_t on, uint16_t off);
		void SetDefaultMode(uint8_t w, uint16_t on, uint16_t off);
		const mode* GetDef(uint8_t w);

		int Update(time_t now, mode* m); // return 1 >> change, 0 >> don't change
		int Update(time_t now);
		const mode* GetOutput();
		int16_t GetModeId();

		//const schedule* GetD(uint16_t idx);
		//void defragment(bool full = true, uint16_t idx = 0);
	private:
		void defragment(bool full = true, uint16_t idx = 0);
		uint16_t _count;

		int16_t _lastId; // id < 0 -> default X; id >= 0 -> _sch[X]

		schedule _sch[MAX_SCHEDLE_COUNT];
		mode _defM[7];
		mode _output;
};

#endif //__SCHEDLE_HPP_

