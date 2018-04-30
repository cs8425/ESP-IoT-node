# [WIP] ESP-Security-IOnode
Using ESP8266 to log and control switch output.
With scheduled output and settable schedule via Web UI.
All setting must be signed by key otherwise will be ignore (WIP, plan to use ECDSA P256 / AES-256 / ChaCha20).

* preallocated:
	* sersor logs
	* schedules


### TODO
- [ ] API
  - [x] output mode
  - [x] schedule output
  - [x] log dump
  - [ ] task
  - [x] wifi/esp setting
- [ ] Web UI
  - [x] schedule output setting
  - [ ] log viewer
  - [ ] task setting
  - [x] wifi/esp setting
- [ ] signature for any setting
  - [x] schedule
  - [x] wifi/esp setting
  - [x] password/key
  - [ ] task
- [ ] save/load to SPIFFS
  - [x] settings
  - [ ] schedule
  - [ ] task
- [ ] multi output pin with schedule
- [x] ~~random number generator (entropy pool)~~ use `os_get_random()` from SDK
- [ ] Wifi scan add join via Web UI


