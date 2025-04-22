#include "emulator.h"

#define VERTICAL_BLANK_SCAN_LINE 0x90
#define VERTICAL_BLANK_SCAN_LINE_MAX 0x99
#define RETRACE_START 456
#define MAX_ROM_SIZE 0x200000
#define CYCLES_PER_FRAME 10
#define FPS 60


Emulator::Emulator() {
}

void Emulator::set_render_func(RenderFunction rf) {
    render_func = rf;
}

CPU Emulator::get_cpu() {
    return cpu;
}

LCD Emulator::get_lcd() {
    return lcd;
}

PPU Emulator::get_ppu() {
    return ppu;
}

bool Emulator::load_rom(const string& rom_name) {
    BYTE rom[MAX_ROM_SIZE];

    FILE* fin;
    fin = fopen(rom_name.c_str(), "rb");
    fread(rom, 1, MAX_ROM_SIZE, fin);
    fclose(fin);

    cpu = CPU(rom);
    lcd = LCD();
    ppu = PPU();

    return true;
}

void Emulator::emulator_update() {
    cycles_this_update = 0 ;
	const int target_cycles = 70221 ;
    while (cycles_this_update < target_cycles) {
 		int cycles = cpu.exec() ;
 		cycles_this_update += cycles;
        lcd.update(cpu, ppu, cycles);
    }
    render_func();
}