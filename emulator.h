#pragma once

#include <stdlib.h>
#include <string>
#include "cpu.h"
#include "lcd.h"
#include "ppu.h"

typedef std::string string;
typedef void (*RenderFunction)() ;

enum KEY {
    
};

class Emulator {
    public:
        Emulator();
        void set_render_func(RenderFunction rf);
        bool load_rom(const string& rom);
        void emulator_update();
        CPU get_cpu();
        LCD get_lcd();
        PPU get_ppu();
    private:
        CPU cpu;
        LCD lcd;
        PPU ppu;
        int cycles_this_update;
        RenderFunction render_func;
        
};
