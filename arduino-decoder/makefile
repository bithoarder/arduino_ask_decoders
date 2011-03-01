CC=avr-gcc
CXX=avr-g++
OBJCOPY=avr-objcopy
AVRDUDE=avrdude
UCOM=../tools/ucom
SERPORT=/dev/ttyACM0
SERSPEED=115200

INCLUDE_PATHS=-Iwiring_0022 -Ilibraries_0022/SPI -Ilibraries_0022/Ethernet
CC_FLAGS=-MMD -MP --pipe -Wall -Werror -ffunction-sections -fdata-sections -Os -g $(INCLUDE_PATHS) -DF_CPU=16000000L -mmcu=atmega328p
CXX_FLAGS=$(CC_FLAGS) -fno-exceptions -fno-rtti
LD_FLAGS=-Wl,--gc-sections -mmcu=atmega328p

ARDUINO_SOURCES = \
	wiring_0022/main.cpp \
	wiring_0022/pins_arduino.c \
	wiring_0022/wiring.c \
	wiring_0022/wiring_digital.c \
	wiring_0022/HardwareSerial.cpp \
	wiring_0022/Print.cpp \
	wiring_0022/WInterrupts.c \
	libraries_0022/Ethernet/utility/w5100.cpp \
	libraries_0022/Ethernet/utility/socket.cpp \
	libraries_0022/SPI/SPI.cpp \

# ignore wawrnings in the arduino wiring isources:
CC_FLAGS_pins_arduino.c = -w -Wno-all -Wno-error
CC_FLAGS_wiring.c = -w -Wno-all -Wno-error
CC_FLAGS_wiring_digital.c = -w -Wno-all -Wno-error
CC_FLAGS_HardwareSerial.cpp = -w -Wno-all -Wno-error
CC_FLAGS_Print.cpp = -w -Wno-all -Wno-error
CC_FLAGS_WInterrupts.c = -w -Wno-all -Wno-error
CC_FLAGS_w5100.cpp = -w -Wno-all -Wno-error
CC_FLAGS_socket.cpp = -w -Wno-all -Wno-error

SOURCES = \
	main.cpp \
	decode.cpp \
	couchdb.cpp \
	decoders/norgo_nge101.cpp \

all: build/decoder.ihex

build/%.o: %.c makefile
	@echo Compiling $<
	@mkdir -p $$(dirname $@)
	$(CC) $(CC_FLAGS) $(CC_FLAGS_$(notdir $<)) -c $< -o $@

build/%.o: %.cpp makefile
	@echo Compiling $<
	@mkdir -p $$(dirname $@)
	$(CXX) $(CXX_FLAGS) $(CC_FLAGS_$(notdir $<)) $(CXX_FLAGS_$(notdir $<)) -c $< -o $@

%.ihex: %.elf
	@echo Hexing $@
	@$(OBJCOPY) -O ihex -R .eeprom $^ $@
	
build/decoder.elf: $(addprefix build/, $(addsuffix .o, $(basename $(SOURCES) $(ARDUINO_SOURCES))))
	@echo Linking $@
	@$(CXX) $^ $(LD_FLAGS) -o $@ 
	
upload:: build/decoder.ihex
	@echo Uploading $<
	@$(AVRDUDE) -patmega328p -carduino -P$(SERPORT) -b$(SERSPEED) -Uflash:w:$<:i

tail::
	@$(UCOM) $(SERPORT) $(SERSPEED)

clean: 
	@rm -rfv build

-include $(addprefix build/, $(addsuffix .d, $(basename $(SOURCES))))

