# Hydro Node - Section 1 firmware
#
# PlatformIO (platformio.ini) is the primary build. This Makefile is the
# dependency-light alternative: it needs only avr-gcc, avr-libc and a copy of
# the Arduino AVR core on disk, which makes it the easy one to run in CI or on a
# machine that cannot reach the PlatformIO registry.
#
#   sudo apt install gcc-avr avr-libc arduino-core-avr
#   make            build ELF + HEX, print size
#   make clean
#
# Override the core location if yours differs:
#   make ARDUINO_AVR=/path/to/hardware/arduino/avr

ARDUINO_AVR ?= /usr/share/arduino/hardware/arduino/avr
CORE        := $(ARDUINO_AVR)/cores/arduino
VARIANT     := $(ARDUINO_AVR)/variants/eightanaloginputs

MCU   := atmega328p
F_CPU := 8000000L

BUILD := build
TARGET := $(BUILD)/hydro_node

# Only the core translation units this firmware actually pulls in. Building the
# whole core would also work; this keeps the build fast and the link map small.
CORE_C   := $(CORE)/wiring.c $(CORE)/wiring_digital.c $(CORE)/wiring_analog.c \
            $(CORE)/hooks.c
CORE_CXX := $(CORE)/main.cpp $(CORE)/Print.cpp $(CORE)/Stream.cpp \
            $(CORE)/HardwareSerial.cpp $(CORE)/HardwareSerial0.cpp \
            $(CORE)/WMath.cpp $(CORE)/WString.cpp $(CORE)/abi.cpp \
            $(CORE)/new.cpp

SRC_CXX  := $(wildcard src/*.cpp)

CPPFLAGS := -DF_CPU=$(F_CPU) -DARDUINO=10816 -DARDUINO_AVR_PRO \
            -DARDUINO_ARCH_AVR \
            -I include -I $(CORE) -I $(VARIANT)

COMMON   := -mmcu=$(MCU) -Os -g -flto -ffunction-sections -fdata-sections \
            -funsigned-char -funsigned-bitfields -fpack-struct -fshort-enums
CXXBASE  := $(COMMON) -std=gnu++11 -fno-exceptions -fno-threadsafe-statics

# The Arduino core is third-party code we do not maintain: build it quietly.
CORE_CFLAGS   := $(COMMON) -std=gnu11 -w
CORE_CXXFLAGS := $(CXXBASE) -w -fpermissive

# Our own code is held to a stricter standard, and warnings are errors.
SRC_CXXFLAGS  := $(CXXBASE) -Wall -Wextra -Wshadow -Werror

LDFLAGS  := -mmcu=$(MCU) -Os -flto -Wl,--gc-sections

OBJ := $(patsubst %,$(BUILD)/core/%.o,$(notdir $(CORE_C))) \
       $(patsubst %,$(BUILD)/core/%.o,$(notdir $(CORE_CXX))) \
       $(patsubst src/%.cpp,$(BUILD)/src/%.o,$(SRC_CXX))

.PHONY: all clean size test
all: $(TARGET).hex size

# Host tests for the pure-logic units. No AVR toolchain involved: the same
# sources that ship to the device are compiled for the workstation against a
# shim Arduino.h.
test:
	@mkdir -p $(BUILD)/tests
	@g++ -std=c++11 -Wall -Wextra -Wshadow -Werror -O1 \
	     -I tests/shim -I include \
	     tests/test_filter.cpp src/hn_filter.cpp src/hn_crc8.cpp \
	     -o $(BUILD)/tests/logic_tests
	@$(BUILD)/tests/logic_tests

$(BUILD)/core/%.c.o: $(CORE)/%.c | dirs
	@echo "  CC   core/$(notdir $<)"
	@avr-gcc $(CPPFLAGS) $(CORE_CFLAGS) -c $< -o $@

$(BUILD)/core/%.cpp.o: $(CORE)/%.cpp | dirs
	@echo "  CXX  core/$(notdir $<)"
	@avr-g++ $(CPPFLAGS) $(CORE_CXXFLAGS) -c $< -o $@

$(BUILD)/src/%.o: src/%.cpp | dirs
	@echo "  CXX  $<"
	@avr-g++ $(CPPFLAGS) $(SRC_CXXFLAGS) -c $< -o $@

$(TARGET).elf: $(OBJ)
	@echo "  LD   $@"
	@avr-gcc $(LDFLAGS) -o $@ $(OBJ) -lm

$(TARGET).hex: $(TARGET).elf
	@avr-objcopy -O ihex -R .eeprom $< $@

size: $(TARGET).elf
	@avr-size --format=avr --mcu=$(MCU) $(TARGET).elf

dirs:
	@mkdir -p $(BUILD)/core $(BUILD)/src

clean:
	rm -rf $(BUILD)
