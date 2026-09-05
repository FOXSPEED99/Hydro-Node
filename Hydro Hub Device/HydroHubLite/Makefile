# Host tests for the parts of the Hub that do not need an ESP32.
#
#   make test
#
# The TFT and radio layers need the ESP32 toolchain and are built in the
# Arduino IDE; what is tested here is the maths and the wire format, which is
# where a wrong answer would be silent rather than obvious.

.PHONY: test clean check-protocol

NODE := ../../firmware/HydroNode

# See the same target in firmware/Makefile - the wire format lives in two
# sketch folders and must never differ.
check-protocol:
	@cmp -s hn_packet.h "$(NODE)/hn_packet.h" && cmp -s hn_packet.cpp "$(NODE)/hn_packet.cpp" \
	  && echo "  protocol OK (Node and Hub copies match)" \
	  || ( echo "  *** PROTOCOL MISMATCH with $(NODE) ***"; exit 1 )

test: check-protocol
	@mkdir -p build
	@g++ -std=c++11 -Wall -Wextra -Wshadow -Werror -O1 \
	     tests/test_tank.cpp TankMath.cpp hn_packet.cpp -o build/tank_tests -lm
	@./build/tank_tests

clean:
	rm -rf build

