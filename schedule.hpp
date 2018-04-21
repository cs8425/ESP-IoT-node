#ifndef __SCHEDLE_HPP_
#define __SCHEDLE_HPP_

#define MAX_MODE_COUNT 32

struct mode {
	// mode id
	uint8_t mid;

	// on time
	unsigned int on : 12;

	// off time
	unsigned int off : 12;
} __attribute__((packed));

class Mode {
	public:
		Mode(int pin);

		//mode* ListMode();
		uint8_t AddMode(uint16_t on, uint16_t off);
		bool RemoveMode(uint8_t mid);
		mode* GetMode();
		uint8_t Count();

		void SetMode(mode m);
		void SetMode(uint8_t mid);

		void Update();

	private:
		int _pin;
		mode temp;
		mode _m[MAX_MODE_COUNT];

};


#define MAX_SCHEDLE_COUNT 168

struct daytime {
	// week 0~6
	unsigned int week : 3;

	// hr 0~23
	unsigned int hr : 5;

	// min 0~60
	unsigned int min : 6;

	// sec 0~60
	unsigned int sec : 6;

} __attribute__((packed));

struct schedule {
	// schedule id
	uint16_t sid;

	// mode id
	uint8_t mid;

	// start
	daytime start;

	// end
	daytime end;
};

class Scheduler {
	public:
		//Scheduler(uint16_t MaxSize = MAX_SCHEDLE_COUNT);
		Scheduler();

		//schedule* ListSchedule();
		uint16_t AddSchedule(daytime a, daytime b, uint8_t mid);
		bool RemoveSchedule(uint16_t sid);
		schedule* GetSchedule();
		uint16_t Count();

		void SetDefaultMode(uint8_t mid);
		void SetDefaultMode(uint8_t w, uint8_t mid);

		uint8_t Update(); // return mid

	private:
		uint16_t _maxsize;
		uint16_t _count;
		schedule _sch[MAX_SCHEDLE_COUNT];
		uint8_t _defM[7];
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
		task_t* GetTask();
		uint8_t Count();

		uint8_t Update(); // return mid

	private:
		uint8_t _maxsize;
		uint8_t _count;
		task_t _tasks[MAX_TASK_COUNT];

};

#endif //__SCHEDLE_HPP_

