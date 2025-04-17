#include <iostream>
#include <vector>
#include <memory>

#include "debug.h"
// #include "globals.h"

#define MAX_ROM_SIZE 0x200000
#define PC_START 0x100

typedef unsigned char BYTE;
typedef char SIGNED_BYTE;
typedef unsigned short WORD;
typedef signed short SIGNED_WORD;

BYTE rom[MAX_ROM_SIZE];
uint16_t pc;

uint16_t exec_opc(BYTE opc) {
    switch(opc) {
        case 0x00:
            d_missing(opc);
            return pc + 1;
        case 0x01:

        case 0xCB:
            std::cerr << "cb inst" << std::endl;
            exit(-1);
            return 0;
        default:
            std::cerr << "meow?" << std::endl;
            exit(-1);
            return 0;
    }
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("usage: %s <Game Boy executable file name>\n", argv[0]);
        exit(-1);
    }

    FILE* fin;
    fin = fopen(argv[1], "rb");
    fread(rom, 1, MAX_ROM_SIZE, fin);
    fclose(fin);

    pc = PC_START;

    /**
     * TODO: change while loop into clock cycling
     */
    int cycles = 0;
    while(cycles < 10) {
        pc = exec_opc(rom[pc]);
        cycles++;
    }

    return 0;
}