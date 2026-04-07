PORT=
PROGRAMMER=stk500v1

upload: build/attiny.avr.ATtinyX5/Keyer.ino.hex
	avrdude -v -c "$(PROGRAMMER)" -P "$(PORT)" -b19200 -pt85 -U "flash:w:$<"

build/attiny.avr.ATtinyX5/Keyer.ino.hex: Keyer.ino
	arduino-cli compile -e
