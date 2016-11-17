ARDUINO_DIR = /usr/share/arduino

BOARD_TAG    = atmega328
ARDUINO_PORT = /dev/ttyUSB0

ARDUINO_LIBS = LiquidCrystal EEPROM

AVR_TOOLS_PATH   = /usr/bin
AVRDUDE_CONF     = /etc/avrdude.conf

include /usr/share/arduino/Arduino.mk
