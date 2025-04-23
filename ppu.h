#pragma once
#include "cpu.h"

class PPU {
    public:
        PPU();
        void renderTiles(CPU& cpu);
        void renderSprites(CPU& cpu);
        void draw(CPU& cpu);
};