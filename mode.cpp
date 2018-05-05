#include "mode.hpp"

Mode::Mode() {
	_status = 0;
}

void Mode::SetMode(uint16_t on, uint16_t off, bool force_reload) {
	reload.on = on;
	reload.off = off;

	if(!force_reload) return;
	temp.on = on;
	temp.off = off;

	_status = 0;
}

void Mode::SetMode(const mode* m, bool force_reload) {
	reload = *m;

	if(!force_reload) return;
	temp = *m;

	_status = 0;
}

int Mode::Update() {

	switch(_status) {
	case 0:
		if (temp.on == 0) {
			_status = 2;
			return 0;
		}
		if (temp.off == 0) {
			_status = 1;
			return 1;
		}
		temp.on -=1;
		_status = 1;
		return 1;
	break;

	case 1:
		if (temp.on == 0) {
			temp.on = reload.on;

			if (temp.off != 0) {
				temp.off -=1;
				_status = 2;
				return 0;
			}
		}
		temp.on -=1;
		return -1;
	break;

	case 2:
		if (temp.off == 0) {
			temp.off = reload.off;

			if (temp.on != 0) {
				temp.on -=1;
				_status = 1;
				return 1;
			}
		}
		temp.off -=1;
		return -1;
	break;
	}

	return -1;
}

int Mode::GetOutput() {
	switch(_status) {
	case 0:
		if (temp.on == 0) {
			return 0;
		}
		if (temp.off == 0) {
			return 1;
		}
	break;


	case 2:
		return 0;
	break;
	}

	return 1;
}

