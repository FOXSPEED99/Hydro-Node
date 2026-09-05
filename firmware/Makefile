# Hydro Node - Section 1 firmware
#
# PlatformIO (platformio.ini) and the Arduino IDE both build the HydroNode
# sketch folder. This Makefile is the dependency-light alternative: it needs
# only avr-gcc, avr-libc and a copy of the Arduino AVR core on disk, which makes
# it the easy one to run in CI or on a machine that cannot reach the PlatformIO
# registry.
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
# SPI ships with the Arduino AVR core; the LoRa driver is the only user.
SPILIB      := $(ARDUINO_AVR)/libraries/SPI/src

MCU   := atmega328p
F_CPU := 8000000L

BUILD := build
TARGET := $(BUILD)/hydro_node

# Only the core translation units this firmware actually pulls in. Building the
# whole core would also work; this keeps the build fast and the link map small.
CORE_C   := $(CORE)/wiring.c $(CORE)/wiring_digital.c $(CORE)/wiring_analog.c \
            $(CORE)/hooks.c
CORE_CXX := $(SPILIB)/SPI.cpp $(CORE)/main.cpp $(CORE)/Print.cpp $(CORE)/Stream.cpp \
            $(CORE)/HardwareSerial.cpp $(CORE)/HardwareSerial0.cpp \
            $(CORE)/WMath.cpp $(CORE)/WString.cpp $(CORE)/abi.cpp \
            $(CORE)/new.cpp

SKETCH   := HydroNode
SRC_CXX  := $(wildcard $(SKETCH)/*.cpp)
SRC_INO  := $(SKETCH)/$(SKETCH).ino

CPPFLAGS := -DF_CPU=$(F_CPU) -DARDUINO=10816 -DARDUINO_AVR_PRO \
            -DARDUINO_ARCH_AVR \
            -I $(SKETCH) -I $(CORE) -I $(VARIANT) -I $(SPILIB)

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
       $(patsubst $(SKETCH)/%.cpp,$(BUILD)/src/%.o,$(SRC_CXX)) \
       $(BUILD)/src/$(SKETCH).ino.o

.PHONY: all clean size test check-protocol
all: check-protocol $(TARGET).hex size

HUB := ../Hydro Hub Device/HydroHubLite

# The wire format exists in two places because the Arduino IDE needs every file
# inside its own sketch folder. Two copies of a protocol definition drift, and
# when this one drifts the symptom is a Hub that silently misreads a field
# rather than anything that looks like an error. So the build refuses to
# proceed unless they are byte-identical.
check-protocol:
	@cmp -s $(SKETCH)/hn_packet.h   "$(HUB)/hn_packet.h"   && \
	 cmp -s $(SKETCH)/hn_packet.cpp "$(HUB)/hn_packet.cpp" && \
	 echo "  protocol OK (Node and Hub copies match)" || \
	 ( echo ""; \
	   echo "  *** PROTOCOL MISMATCH ***"; \
	   echo "  $(SKETCH)/hn_packet.* and the Hub's copy differ."; \
	   echo "  Edit one, then copy it over the other:"; \
	   echo "    cp $(SKETCH)/hn_packet.h   \"$(HUB)/hn_packet.h\""; \
	   echo "    cp $(SKETCH)/hn_packet.cpp \"$(HUB)/hn_packet.cpp\""; \
	   echo ""; exit 1 )

# Host tests for the pure-logic units. No AVR toolchain involved: the same
# sources that ship to the device are compiled for the workstation against a
# shim Arduino.h.
test: check-protocol
	@mkdir -p $(BUILD)/tests
	@g++ -std=c++11 -Wall -Wextra -Wshadow -Werror -O1 \
	     -I tests/shim -I $(SKETCH) \
	     tests/test_filter.cpp $(SKETCH)/hn_filter.cpp $(SKETCH)/hn_crc8.cpp \
	     $(SKETCH)/hn_packet.cpp $(SKETCH)/hn_telemetry.cpp $(SKETCH)/hn_reading.cpp \
	     -o $(BUILD)/tests/logic_tests
	@$(BUILD)/tests/logic_tests

$(BUILD)/core/%.c.o: $(CORE)/%.c | dirs
	@echo "  CC   core/$(notdir $<)"
	@avr-gcc $(CPPFLAGS) $(CORE_CFLAGS) -c $< -o $@

$(BUILD)/core/%.cpp.o: $(CORE)/%.cpp | dirs
	@echo "  CXX  core/$(notdir $<)"
	@avr-g++ $(CPPFLAGS) $(CORE_CXXFLAGS) -c $< -o $@

$(BUILD)/core/%.cpp.o: $(SPILIB)/%.cpp | dirs
	@echo "  CXX  lib/$(notdir $<)"
	@avr-g++ $(CPPFLAGS) $(CORE_CXXFLAGS) -c $< -o $@

$(BUILD)/src/%.o: $(SKETCH)/%.cpp | dirs
	@echo "  CXX  $<"
	@avr-g++ $(CPPFLAGS) $(SRC_CXXFLAGS) -c $< -o $@

# The sketch file. It is a normal translation unit - it includes Arduino.h and
# declares everything it uses - so it needs no prototype generation and can be
# compiled directly, with -x c++ only to tell the compiler what .ino means.
$(BUILD)/src/$(SKETCH).ino.o: $(SRC_INO) | dirs
	@echo "  CXX  $<"
	@avr-g++ $(CPPFLAGS) $(SRC_CXXFLAGS) -x c++ -c $< -o $@

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
