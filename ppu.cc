#include "ppu.h"
#include <stdio.h>

PPU::PPU () {
}

inline int getColor(CPU& cpu, WORD addr, int colorId) {
    BYTE palette = cpu.read_mem(addr);
    return (((palette >> (colorId * 2 + 1)) & 0b1) << 1) | ((palette >> (colorId * 2)) & 0b1);
}

void PPU::renderTiles(CPU& cpu) {
    BYTE LCDCR = cpu.read_mem(0xFF40); // LCD Control Register

    WORD background;
    BYTE currLine;

    BYTE SCY = cpu.read_mem(0xFF42);
    BYTE SCX = cpu.read_mem(0xFF43);

    BYTE WY = cpu.read_mem(0xFF4A);
    BYTE WX = cpu.read_mem(0xFF4B) - 7;

    WORD tileData = LCDCR & 0b10000 ? 0x8000 : 0x8800;
    BYTE scanline = cpu.read_mem(0xFF44);
    bool windowOn = (LCDCR & 0b100000) && WY <= scanline; // check window display enable bit

    if (windowOn) {
        background = LCDCR & 0b1000000 ? 0x9C00 : 0x9800;
        currLine = scanline - WY;
    } else {
        background = LCDCR & 0b1000 ? 0x9C00 : 0x9800;
        currLine = scanline + SCY;
    }
    WORD row = currLine / 8 * 32;

    // cache color values
    int colors[4];
    for (int i = 0; i < 4; i++) {
        colors[i] = getColor(cpu, 0xFF47, i);
    }
    
    for (int i = 0; i < 160; i++) {
        BYTE currCol = windowOn ? i - WX : i + SCX;
        WORD tileMem;

        if (LCDCR & 0b10000) {
            tileMem = tileData + cpu.read_mem(background + row + currCol / 8) * 16;
        } else {
            tileMem = tileData + ((SIGNED_BYTE) cpu.read_mem(background + row + currCol / 8) + 128) * 16;
        }

        BYTE data1 = cpu.read_mem(tileMem + 2 * (currLine % 8));
        BYTE data2 = cpu.read_mem(tileMem + 2 * (currLine % 8) + 1);

        int colorBit = 7 - currCol % 8;
        int colorId = (((data2 >> colorBit) & 0b1) << 1) | ((data1 >> colorBit) & 0b1);
        int color = colors[colorId];

        switch (color) {
            case 0:
                screen[scanline][i][0] = 255;
                screen[scanline][i][1] = 255;
                screen[scanline][i][2] = 255;
                break;
            case 1:
                screen[scanline][i][0] = 0xCC;
                screen[scanline][i][1] = 0xCC;
                screen[scanline][i][2] = 0xCC;
                break;
            case 2:
                screen[scanline][i][0] = 0x77;
                screen[scanline][i][1] = 0x77;
                screen[scanline][i][2] = 0x77;
                break;
            case 3:
                screen[scanline][i][0] = 0x0;
                screen[scanline][i][1] = 0x0;
                screen[scanline][i][2] = 0x0;
                break;
        }
    }
}

void PPU::renderSprites(CPU& cpu) {
    BYTE LCDCR = cpu.read_mem(0xFF40); // LCD Control Register

    // cache color values
    int colors1[4];
    int colors2[4];
    for (int i = 0; i < 4; i++) {
        colors1[i] = getColor(cpu, 0xFF48, i);
        colors2[i] = getColor(cpu, 0xFF49, i);
    }

    int spriteCounter = 0;
    for (int i = 0; i < 40; i++) {
        if (spriteCounter >= 10) {
            break;
        }

        BYTE idx = i * 4;
        BYTE XPos = cpu.read_mem(idx + 0xFE00 + 1) - 8;
        BYTE YPos = cpu.read_mem(idx + 0xFE00) - 16;
        BYTE flags = cpu.read_mem(idx + 0xFE00 + 3);
        BYTE scanline = cpu.read_mem(0xFF44);
        int spriteSize = LCDCR & 0b100 ? 16 : 8;

        if (scanline >= YPos && scanline < YPos + spriteSize) {
            spriteCounter++;

            WORD addr = 0x8000 + cpu.read_mem(idx + 0xFE00 + 2) * 16 + 2 * (flags & 0b1000000 ? YPos + spriteSize - scanline - 1 : scanline - YPos);
            BYTE data1 = cpu.read_mem(addr);
            BYTE data2 = cpu.read_mem(addr + 1);

            for (int j = 0; j < 8; j++) {
                int colorBit = flags & 0b100000 ? 7 - j : j;
                int colorId = (((data2 >> colorBit) & 0b1) << 1) | ((data1 >> colorBit) & 0b1);
                int color = flags & 0b10000 ? colors2[colorId] : colors1[colorId];

                // Sprite BG Priority
                if (colorId == 0 || 
                    (flags & 0b10000000 && (screen[scanline][XPos + 7 - j][0] != 255 || screen[scanline][XPos + 7 - j][1] != 255 || screen[scanline][XPos + 7 - j][2] != 255)) || 
                    (XPos + 7 - j <= 0 || XPos + 7 - j >= 160 || scanline <= 0 || scanline >= 144)) {
                    continue;
                }

                switch (color) {
                    case 0:
                        screen[scanline][XPos + 7 - j][0] = 255;
                        screen[scanline][XPos + 7 - j][1] = 255;
                        screen[scanline][XPos + 7 - j][2] = 255;
                        break;
                    case 1:
                        screen[scanline][XPos + 7 - j][0] = 0xCC;
                        screen[scanline][XPos + 7 - j][1] = 0xCC;
                        screen[scanline][XPos + 7 - j][2] = 0xCC;
                        break;
                    case 2:
                        screen[scanline][XPos + 7 - j][0] = 0x77;
                        screen[scanline][XPos + 7 - j][1] = 0x77;
                        screen[scanline][XPos + 7 - j][2] = 0x77;
                        break;
                    case 3:
                        screen[scanline][XPos + 7 - j][0] = 0x0;
                        screen[scanline][XPos + 7 - j][1] = 0x0;
                        screen[scanline][XPos + 7 - j][2] = 0x0;
                        break;
                }
            }
        }
    }
}

void PPU::writePixels(CPU& cpu) {
    for (int i = 0; i < 144; i++) {
        for (int j = 0; j < 160; j++) {
            if (screen[i][j][0] != cpu.screen[i][j][0]) {
                cpu.screen[i][j][0] = screen[i][j][0];
                cpu.screen[i][j][1] = screen[i][j][1];
                cpu.screen[i][j][2] = screen[i][j][2];
                if (i < cpu.dirtyMinY) {
                    cpu.dirtyMinY = i;
                }
                if (i > cpu.dirtyMaxY) {
                    cpu.dirtyMaxY = i;
                }
                if (j < cpu.dirtyMinX) {
                    cpu.dirtyMinX = j;
                }
                if (j > cpu.dirtyMaxX) {
                    cpu.dirtyMaxX = j;
                }
            }
        }
    }
}

void PPU::draw(CPU& cpu) {
    BYTE LCDCR = cpu.read_mem(0xFF40); // LCD Control Register
    if (LCDCR & 0b1) {
        renderTiles(cpu);
    }
    if (LCDCR & 0b10) {
        renderSprites(cpu);
    }
    if (LCDCR & 0b11) {
        writePixels(cpu);
    }
}