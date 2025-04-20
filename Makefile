CPP_COMPILER ?= g++
OPT ?= 3
CCFLAGS = -Wall -O${OPT} -g -std=c++17
LDFLAGS = -lSDL -lGL -lGLU

EMULATOR = .run
SCREEN = screen

# Collect all .cc sources and corresponding object files
SRCS := $(wildcard *.cc)
OBJS := $(SRCS:.cc=.o)

.PHONY: all clean $(ROMS)
all: $(EMULATOR)

# Link all object files into the emulator binary
$(EMULATOR): $(OBJS)
	$(CPP_COMPILER) $(CXXFLAGS) -o $@ $(OBJS) 

game: screen.cc
	$(CPP_COMPILER) $(CCFLAGS) screen.cc -o $(SCREEN) $(LDFLAGS)
	./$(SCREEN)

# Compile each .cc into .o
%.o: %.cc
	$(CPP_COMPILER) $(CXXFLAGS) -c $< -o $@

# Run emulator on any .gb ROM: make name.gb
ROMS := $(wildcard *.gb)
.PHONY: $(ROMS)
$(ROMS): %: $(EMULATOR)
	@echo "Running $(EMULATOR) on ROM: $@"
	@/usr/bin/time -f "Elapsed time: %E" ./$(EMULATOR) $@

# Clean up build artifacts
clean:
	-rm -f $(EMULATOR) $(OBJS)