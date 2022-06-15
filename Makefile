MCU=attiny85
PORT ?= /dev/ttyACM0

GAME ?= SpaceAttackAttiny
GAME_DIR = $(realpath ./$(GAME))

BUILD_DIR = $(GAME_DIR)/build
HEX_PATH = $(BUILD_DIR)/$(GAME).ino.hex
ELF_PATH = $(BUILD_DIR)/$(GAME).ino.elf
DIS_PATH = $(BUILD_DIR)/$(GAME).ino.dis


OLED_LIB = $(realpath ./lib/oled)


all: $(HEX_PATH)

$(HEX_PATH):
	mkdir -p $(GAME_DIR)/build
	arduino-cli compile -b attiny:avr:ATtinyX5:cpu=$(MCU),clock=internal8 $(GAME_DIR) --library $(OLED_LIB) --output-dir $(BUILD_DIR)
	avr-objdump -h -S $(ELF_PATH) > $(DIS_PATH)

flash: $(HEX_PATH)
	avrdude -v -p$(MCU) -cstk500v1 -P$(PORT) -b19200 -e -U efuse:w:0xff:m -U hfuse:w:0xdf:m -U lfuse:w:0xe2:m
	avrdude -P $(PORT) -c stk500v1 -b 19200 -p $(MCU) -D -U flash:w:$<:i


clean:
	rm -rf $(BUILD_DIR)


deps:
	sudo apt-get install avr-libc binutils-avr gcc-avr avrdude
	wget -qO arduino-cli.tar.gz https://downloads.arduino.cc/arduino-cli/arduino-cli_latest_Linux_64bit.tar.gz
	sudo tar xf arduino-cli.tar.gz -C /usr/local/bin arduino-cli
	arduino-cli version
	rm -rf arduino-cli.tar.gz
