#———— Variables ——————————————————————————————
CPP_COMPILER := g++
OPT           ?= 3
CXXFLAGS      := -Wall -O${OPT} -g -std=c++17
LDFLAGS       := -lSDL -lGL -lGLU

EMULATOR      := .run
SCREEN        := screen

SRCS          := $(wildcard *.cc)
OBJS          := $(SRCS:.cc=.o)
ROMS          := $(shell find . -type f -name '*.gb')

#———— Phony targets ————————————————————————————
.PHONY: all clean

#———— Default build ——————————————————————————
all: $(EMULATOR)

#———— Link emulator binary ————————————————————
$(EMULATOR): $(OBJS)
	$(CPP_COMPILER) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)

#———— Compile each .cc to .o ———————————————
%.o: %.cc
	$(CPP_COMPILER) $(CXXFLAGS) -c $< -o $@

#———— Run *any* .gb under ./ recursively —————————
%.gb: $(EMULATOR)
	@echo "Running $(EMULATOR) on ROM: $@"
	@/usr/bin/time -f "Elapsed time: %E" ./$(EMULATOR) $@

#———— Clean up —————————————————————————————
clean:
	-rm -f $(EMULATOR) $(OBJS)