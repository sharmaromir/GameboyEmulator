#pragma once
#include "cpu.h"
#include "ppu.h"

class LCD {
    public:
        LCD();
        void update(CPU& cpu, PPU& ppu, int cycles);
        void setMode(CPU& cpu);
    private:
        int slCtr = 456; // scanline counter, 456 clock cycles per one scanline
};