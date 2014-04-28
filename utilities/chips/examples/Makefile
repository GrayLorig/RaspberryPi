CC=gcc
UTILS_DIR = /home/pi/dev/RaspberryPi/utilities
INCLUDE = -I. -I$(UTILS_DIR)/chips
LIBRARIES =
CFLAGS = -c -Wall $(INCLUDE) -Winline -pipe -fPIC
LDFALGS =


CHIPS = MCP23008 MCP3008 MCP4725 TSL2561

EXECUTABLES = $(CHIPS:%=%-test)

all: $(EXECUTABLES)

%-test: %-test.o
	@echo [Link] $@
	@$(CC) $< $(LIBRARIES) -o $@

.cpp.o:
	@echo [Compile] $<
	@$(CC) $(CFLAGS) $< -o $@

clean:
	rm -f $(EXECUTABLES) $(EXECUTABLES:%=%.o)


