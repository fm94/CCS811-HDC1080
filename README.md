# CCS811-HDC1080
Multiple tests with the CCS811 and HDC1080 sensors

Here I collect observations and tests, you may find some usefull code but overall, this is just tinkering.

v4.ino: 
* esp8266
* measure every 60 secs and put sensor to sleep
* put esp866 into periodic light sleep (50 secs), wake up before sensor and wait
* current consumption 22mA (15 + 7) during sleep and 80mA during measurements
* light sleep with interrupt didn't work because esp wakes up late (ccs811 gives wrong values)