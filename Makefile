# SPDX-License-Identifier: Apache-2.0

.PHONY: all menuconfig clean write

all:
	west build -p auto -b adafruit_clue -- -DBOARD_ROOT=.

menuconfig:
	west build -t menuconfig

clean:
	rm -rf build

write: build/zephyr/zephyr.hex
	openocd -f openocd.cfg -c "program $< reset exit"
