# ESP-Security-IoT-node
Using ESP8266 to control relay output which scheduled and settable via Web UI, and log sensor data in the meantime.
All setting must be signed/encrypt by key, otherwise will be ignore (using AES-256).
Try to maximum security without TLS.

* preallocated RAM:
	* sersor logs
	* schedules
	* [WIP] task
* output:
	* single pin
		* total 7*24 rules
		* cycle weekly
		* PWM output, period up to 8192 seconds, minimum interval 1 seconds, just setting ON and OFF seconds.
	* [WIP] multi pin
* sensors:
	* BME280
		* Temperature
		* Humidity
		* Pressure

### Web UI Screenshots

![startup page](https://raw.githubusercontent.com/cs8425/ESP-IoT-node/master/docs/001.png)
----
![setting page](https://raw.githubusercontent.com/cs8425/ESP-IoT-node/master/docs/010.png)
----
![schedule page 1](https://raw.githubusercontent.com/cs8425/ESP-IoT-node/master/docs/022.png)
----
![schedule page 2](https://raw.githubusercontent.com/cs8425/ESP-IoT-node/master/docs/025.png)
----
![set rule](https://raw.githubusercontent.com/cs8425/ESP-IoT-node/master/docs/023.png)
----
![set default rule](https://raw.githubusercontent.com/cs8425/ESP-IoT-node/master/docs/024.png)
----
![log page](https://raw.githubusercontent.com/cs8425/ESP-IoT-node/master/docs/031.png)


### Build

1. install library which I had modified: [ESPAsyncWebServer](https://github.com/cs8425/ESPAsyncWebServer.git)
2. install library [ESPAsyncTCP](https://github.com/me-no-dev/ESPAsyncTCP.git) (skip if already installed)
3. copy `config.example.h` to `config.h`, and edit the value you want to change(eg: wifi config, encrypt key).
4. (optional) edit preallocated size: `MAX_LOG_COUNT`@`sensor_log.hpp`, `MAX_SCHEDLE_COUNT`@`schedule.hpp`
5. (optional) add/remove sensor in `IoT-node.ino`
6. compile & upload code
7. upload web UI data via [arduino-esp8266fs-plugin](https://github.com/esp8266/arduino-esp8266fs-plugin)


### Reset config

1. attach UART(Serial), baud 115200 8-N-1
2. send `RRRRR`, character `R` more than 5 times, you will see `reset all config...`
3. press reset button or power cycled ESP


### TODO
- [ ] API
  - [x] output mode
  - [x] schedule output
  - [x] log dump
  - [ ] task
  - [x] wifi/esp setting
- [ ] Web UI
  - [x] schedule output setting
  - [x] log viewer
  - [ ] task setting
  - [x] wifi/esp setting
- [ ] signature for any setting
  - [x] schedule
  - [x] wifi/esp setting
  - [x] password/key
  - [ ] task
- [ ] save/load to SPIFFS
  - [x] settings
  - [x] schedule
  - [ ] task
- [ ] multi output pin with schedule
- [x] ~~random number generator (entropy pool)~~ use `os_get_random()` from SDK
- [ ] Wifi scan add join via Web UI
- [ ] PID control output with sensor feedback


