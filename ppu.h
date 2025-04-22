#include "cpu.h"

class PPU {
    public:
        void renderTiles(CPU cpu);
        void renderSprites(CPU cpu);
        void draw(CPU cpu);
};