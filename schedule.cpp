#include "schedule.hpp"

Scheduler::Scheduler() {
	Clear();
}

uint16_t Scheduler::Count() {
	return _count;
}

void Scheduler::Clear() {
	unsigned i;
	_count = 0;
	for(i = 0; i<MAX_SCHEDLE_COUNT; i++) {
		_sch[i].start.week = 7;
		_sch[i].start.sec = 0;
		_sch[i].end.week = 7;
		_sch[i].end.sec = 0;
		_sch[i].m.on = 0;
		_sch[i].m.off = 0;
	}
	_lastId = -20;
}

int Scheduler::Add(daytime a, daytime b, uint16_t on, uint16_t off) {
	mode m = { on, off };
	return Add(a, b, m);
}

int Scheduler::Add(daytime a, daytime b, mode m) {
	if (_count + 1 >= MAX_SCHEDLE_COUNT) {
		return -1;
	}

	uint16_t idx = _count;
	_count += 1;

	_sch[idx].start = a;
	_sch[idx].end = b;
	_sch[idx].m.on = m.on;
	_sch[idx].m.off = m.off;

	return idx;
}

bool Scheduler::Mod(uint16_t idx, daytime a, daytime b, mode m) {
	if (idx >= _count) {
		return false;
	}

	_sch[idx].start = a;
	_sch[idx].end = b;
	_sch[idx].m.on = m.on;
	_sch[idx].m.off = m.off;

	return true;
}

bool Scheduler::Del(uint16_t idx) {
	if (idx >= _count) {
		return false;
	}

	_sch[idx].start.week = 7;
	_sch[idx].end.week = 7;
	//_sch[idx].m.on = 0;
	//_sch[idx].m.off = 0;

	// Defragment
	defragment(false, idx);

	return true;
}

void Scheduler::defragment(bool full, uint16_t idx) {
	unsigned i = (full)? 0: idx;
	unsigned end = (full)? MAX_SCHEDLE_COUNT: _count;
	end -= 1;

	for(; i<end; i++) {
		if ((_sch[i].start.week == 7) && (_sch[i+1].start.week != 7)) {
			_sch[i].start.week = _sch[i+1].start.week;
			_sch[i].start.sec = _sch[i+1].start.sec;
			_sch[i].end.week = _sch[i+1].end.week;
			_sch[i].end.sec = _sch[i+1].end.sec;
			_sch[i].m.on = _sch[i+1].m.on;
			_sch[i].m.off = _sch[i+1].m.off;

			_sch[i+1].start.week = 7; // mark remove
		}
	}

	unsigned dec = 0;
	if (full) {
		for(i=_count - 1; i<MAX_SCHEDLE_COUNT; i++) {
			if (_sch[i].start.week == 7 && _sch[i].end.week != 7) {
				_sch[i].start.week = 7;
				_sch[i].start.sec = 0;
				_sch[i].end.week = 7;
				_sch[i].end.sec = 0;
				_sch[i].m.on = 0;
				_sch[i].m.off = 0;
				dec++;
			}
		}
	} else {
		i = end;
		if (_sch[i].start.week == 7) {
			_sch[i].start.week = 7;
			_sch[i].start.sec = 0;
			_sch[i].end.week = 7;
			_sch[i].end.sec = 0;
			_sch[i].m.on = 0;
			_sch[i].m.off = 0;
			dec++;
		}
	}
	_count -= dec;
}

const schedule* Scheduler::Get(uint16_t idx) {
	if (idx >= _count) {
		return nullptr;
	}

	return &_sch[idx];
}

/*const schedule* Scheduler::GetD(uint16_t idx) {
	if (idx >= MAX_SCHEDLE_COUNT) {
		return nullptr;
	}

	return &_sch[idx];
}*/

const mode* Scheduler::GetDef(uint8_t w) {
	if (w >= 7) {
		return nullptr;
	}

	return &_defM[w];
}

void Scheduler::SetDefaultMode(uint16_t on, uint16_t off) {
	unsigned i;
	for(i = 0; i<7; i++) {
		_defM[i].on = on;
		_defM[i].off = off;
	}
}

void Scheduler::SetDefaultMode(uint8_t w, uint16_t on, uint16_t off) {
	if(w > 6) return;

	_defM[w].on = on;
	_defM[w].off = off;
}

int Scheduler::Update(time_t now) {
	return Update(now, &_output);
}

int Scheduler::Update(time_t now, mode* m) {
	unsigned i;

	const tm* tm = localtime(&now);

	unsigned nw = tm->tm_wday; // week 0~6
	unsigned ns = (tm->tm_hour * 3600) + (tm->tm_min * 60) + tm->tm_sec;

	m->on = _defM[nw].on;
	m->off = _defM[nw].off;

	int16_t newId = -10 - nw;

	for(i = 0; i<MAX_SCHEDLE_COUNT; i++) {
		if (_sch[i].start.week != nw) {
			continue;
		}

		if ((_sch[i].start.sec <= ns) && (ns < _sch[i].end.sec)) {
			m->on = _sch[i].m.on;
			m->off = _sch[i].m.off;
			newId = i;
		}
	}


	if (_lastId != newId) {
		_lastId = newId;
		return 1;
	}

	return 0;
}

const mode* Scheduler::GetOutput() {
	return &_output;
}


