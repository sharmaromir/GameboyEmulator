#pragma once
#include "cpu.h"

class PPU {
    public:
        PPU();
        BYTE screen[144][160][3];
        void renderTiles(CPU& cpu);
        void renderSprites(CPU& cpu);
        void writePixels(CPU& cpu);
        void draw(CPU& cpu);
};