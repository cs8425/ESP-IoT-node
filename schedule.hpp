#ifndef __SCHEDLE_HPP_
#define __SCHEDLE_HPP_

#include <stdint.h>
#include <time.h>

struct mode {
	// on time
	unsigned int on : 12;

	// off time
	unsigned int off : 12;
} __attribute__((packed));

class Mode {
	public:
		Mode();
		void SetMode(uint16_t on, uint16_t off, bool force_reload = true);
		int Update(); // 0 >> off, 1 >> on, -1 >> don't change

	private:
		uint8_t _status; // 0 >> dec on, 1 >> dec off

		mode temp;
		mode reload;
};


#define MAX_SCHEDLE_COUNT 168

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
		bool Del(uint16_t idx);
		const schedule* Get(uint16_t idx);
		uint16_t Count();
		void Clear();

		void SetDefaultMode(uint16_t on, uint16_t off);
		void SetDefaultMode(uint8_t w, uint16_t on, uint16_t off);

		const mode* Update(time_t now); // return mode

		//const schedule* GetD(uint16_t idx);
		//void defragment(bool full = true, uint16_t idx = 0);
	private:
		void defragment(bool full = true, uint16_t idx = 0);
		uint16_t _count;
		schedule _sch[MAX_SCHEDLE_COUNT];
		mode _defM[7];
};



#define MAX_TASK_COUNT 16

struct task_t {
	// task id
	uint8_t tid;

	// mode id
	uint8_t mid;

	// delay
	uint16_t delay;
} __attribute__((packed));

class Task {
	public:
		//Task(uint8_t MaxSize = MAX_TASK_COUNT);
		Task();

		//task_t* ListTask();
		uint8_t AddTask(uint16_t delay, uint8_t mid);
		bool RemoveTask(uint8_t tid);
		const task_t* GetTask(uint8_t tid);
		uint8_t Count();

		uint8_t Update(); // return mid

	private:
		uint8_t _maxsize;
		uint8_t _count;
		task_t _tasks[MAX_TASK_COUNT];

};

#endif //__SCHEDLE_HPP_

