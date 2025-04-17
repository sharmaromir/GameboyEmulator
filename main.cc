#include <iostream>

#include "cpu.h"

#define MAX_ROM_SIZE 0x200000

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("usage: %s <Game Boy executable file name>\n", argv[0]);
        exit(-1);
    }
    printf("loading rom\n");

    BYTE rom[MAX_ROM_SIZE];

    FILE* fin;
    fin = fopen(argv[1], "rb");
    fread(rom, 1, MAX_ROM_SIZE, fin);
    fclose(fin);

    CPU cpu(rom);

    printf("starting\n");
    /**
     * TODO: change while loop into clock cycling
     */
    int cycles = 0;
    while(cycles < 100) {
        // printf("PC: %04X\n", rom[PC]);
        std::cerr << "cycle " << cycles << std::endl;
        cycles += cpu.exec();
        // printf("in loop\n");
    }

    return 0;
}