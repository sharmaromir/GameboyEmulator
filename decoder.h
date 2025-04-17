#pragma once

#include <string>
#include <vector>
#include <memory>

#include "globals.h"

struct Inst {
    WORD opcode;
};

class Decoder {
    BYTE* rom;
public:
    Decoder(BYTE* rom) : rom(rom) {};
    Inst next();
};