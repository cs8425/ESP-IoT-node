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
  - [ ] wifi/esp setting
- [ ] Web UI
  - [x] schedule output setting
  - [ ] log viewer
  - [ ] task setting
  - [ ] wifi/esp setting
- [ ] signature for any setting
  - [x] schedule
  - [ ] wifi
  - [ ] password/key
- [ ] save/load to SPIFFS
  - [ ] settings
  - [ ] schedule
  - [ ] task
- [ ] multi output pin with schedule
- [ ] random number generator (entropy pool)
- [ ] Wifi scan add join via Web UI


