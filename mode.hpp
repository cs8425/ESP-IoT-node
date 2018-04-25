#ifndef __MODE_HPP_
#define __MODE_HPP_

#include <stdint.h>

struct mode {
	// on time (s)
	unsigned int on : 12;

	// off time (s)
	unsigned int off : 12;
} __attribute__((packed));

class Mode {
	public:
		Mode();
		void SetMode(uint16_t on, uint16_t off, bool force_reload = true);
		void SetMode(const mode* m, bool force_reload = true);
		int Update(); // 0 >> off, 1 >> on, -1 >> don't change
		int GetOutput(); // 0 >> off, 1 >> on

	private:
		uint8_t _status; // 0 >> dec on, 1 >> dec off

		mode temp;
		mode reload;
};

#endif //__MODE_HPP_

