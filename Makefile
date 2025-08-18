all:
	platformio run

fix:
	rm .pio/libdeps/adafruit_itsybitsy_m0/MS56XX/src/main.cpp
