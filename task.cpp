#include "task.hpp"

Task::Task() {
	Clear();
}

uint8_t Task::Count() {
	return _count;
}

void Task::Clear() {
	unsigned i;
	_count = 0;
	for(i = 0; i<MAX_TASK_COUNT; i++) {
		_tasks[i].delay = 0;
		_tasks[i].en = 0;
		_tasks[i].keep = 0;
		_tasks[i].rm = 1;
		_tasks[i].m.on = 0;
		_tasks[i].m.off = 0;
	}
	_lastId = -1;
}

int Task::Add(uint32_t delay, uint32_t keep, mode m) {
	if (_count + 1 >= MAX_TASK_COUNT) {
		return -1;
	}

	uint16_t idx = _count;
	_count += 1;

	_tasks[idx].delay = delay;
	_tasks[idx].en = 1;
	_tasks[idx].keep = keep;
	_tasks[idx].rm = 0;
	_tasks[idx].m.on = m.on;
	_tasks[idx].m.off = m.off;

	return idx;
}

bool Task::Del(uint8_t idx) {
	if (idx >= _count) {
		return false;
	}

	_tasks[idx].en = 0;
	_tasks[idx].rm = 1;
/*	_tasks[idx].delay = 0;
	_tasks[idx].keep = 0;
	_tasks[idx].m.on = 0;
	_tasks[idx].m.off = 0;*/

	// Defragment
	defragment(false, idx);

	return true;
}

void Task::defragment(bool full, uint8_t idx) {
	unsigned i = (full)? 0: idx;
	unsigned end = (full)? MAX_TASK_COUNT: _count;
	end -= 1;

	for(; i<end; i++) {
		if ((_tasks[i].rm == 1) && (_tasks[i+1].rm == 0)) {
			_tasks[i].delay = _tasks[i+1].delay;
			_tasks[i].en = _tasks[i+1].en;
			_tasks[i].keep = _tasks[i+1].keep;
			_tasks[i].rm = 0;
			_tasks[i].m.on = _tasks[i+1].m.on;
			_tasks[i].m.off = _tasks[i+1].m.off;

			_tasks[i+1].rm = 1; // mark remove
		}
	}

	if (full) {
		for(i=_count - 1; i<MAX_TASK_COUNT; i++) {
			_tasks[i].delay = 0;
			_tasks[i].en = 0;
			_tasks[i].keep = 0;
			_tasks[i].rm = 1;
			_tasks[i].m.on = 0;
			_tasks[i].m.off = 0;
		}
	}
	_count -= 1;
}

const task_t* Task::Get(uint8_t idx) {
	if (idx >= _count) {
		return nullptr;
	}

	return &_tasks[idx];
}

const task_t* Task::GetD(uint8_t idx) {
	if (idx >= MAX_TASK_COUNT) {
		return nullptr;
	}

	return &_tasks[idx];
}

int Task::Update() {
	unsigned i;
	int16_t newId = -1;

	for(i = 0; i<MAX_TASK_COUNT; i++) {
		if (!_tasks[i].en) {
			continue;
		}
		if (_tasks[i].delay > 0) {
			_tasks[i].delay -= 1;
		}
	}

	for(i = 0; i<MAX_TASK_COUNT; i++) {
		if ((!_tasks[i].en) || (_tasks[i].delay != 0)){
			continue;
		}
		if (_tasks[i].keep > 0) {
			_tasks[i].keep -= 1;
			_output.on = _tasks[i].m.on;
			_output.off = _tasks[i].m.off;
			newId = i;
			break;
		}
		if (_tasks[i].keep == 0) {
			_tasks[i].en = 0;
			_tasks[i].rm = 1;
			_output.on = 0;
			_output.off = 0;
		}
	}


	if (_lastId != newId) {
		_lastId = newId;
		return 1;
	}

	return 0;
}

const mode* Task::GetOutput() {
	return &_output;
}

