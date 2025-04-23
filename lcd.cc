#include "lcd.h"
#include <stdio.h>

LCD::LCD() {
}

void LCD::update(CPU& cpu, PPU& ppu, int cycles) {
    setMode(cpu);
    if (cpu.read_mem(0xFF40) & 0b10000000) { // check lcd enable bit
        slCtr -= cycles; // update scanline counter
        BYTE currLine = cpu.read_mem(0xFF44); // get current scanline
        if (currLine > 153) {
            cpu.write_mem(0xFF44, 0); // reset scanline
            currLine = 0;
        }
        if (slCtr <= 0) {
            slCtr = 456; // reset scanline counter
            if (currLine < 144) {
                ppu.draw(cpu);
            } else if (currLine == 144) { // vblank
                cpu.interrupt(0);
            }
            cpu.write_mem(0xFF44, currLine + 1); // go to next scanline
        }
    }
}

void LCD::setMode(CPU& cpu) {
    BYTE LCDSR = cpu.read_mem(0xFF41); // LCD Status Register
    if (cpu.read_mem(0xFF40) & 0b10000000) { // check lcd enable bit
        bool modeChanged = false;
        BYTE currLine = cpu.read_mem(0xFF44); // get current scanline
        BYTE currMode = LCDSR & 0b11; // get current mode
        if (currLine >= 144) { // mode 1
            modeChanged = (LCDSR & 0b10000) && currMode != 1;
            LCDSR = (LCDSR & 0b11111101) | 0b1; // set lower 2 bits to 1
        } else if (slCtr >= 456 - 80) { // mode 2
            modeChanged = (LCDSR & 0b100000) && currMode != 2;
            LCDSR = (LCDSR & 0b11111110) | 0b10; // set lower 2 bits to 2
        } else if (slCtr >= 456 - 80 - 172) { // mode 3
            modeChanged = false;
            LCDSR = LCDSR  | 0b11; // set lower 2 bits to 3
        } else { // mode 0
            modeChanged = (LCDSR & 0b1000) && currMode != 0;
            LCDSR = LCDSR & 0b11111100; // set lower 2 bits to 0
        }
        if (modeChanged) {
            cpu.interrupt(1);
        }
        if (currLine == cpu.read_mem(0xFF45)) { // Coincidence Flag
            LCDSR |= 0b100;
            if (LCDSR & 0b01000000) {
                cpu.interrupt(1);
            }
        } else {
            LCDSR &= 0b11111011;
        }
        cpu.write_mem(0xFF41, LCDSR);
    } else {
        slCtr = 456; // reset scanline counter
        cpu.write_mem(0xFF44, 0); // reset scanline
        cpu.write_mem(0xFF41, (LCDSR & 0b11111101) | 0b1);
    }
}