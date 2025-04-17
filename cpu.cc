#include <iostream>
#include <vector>
#include <memory>

#include "debug.h"
#include "decoder.h"
#include "globals.h"

BYTE rom[MAX_ROM_SIZE];

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("usage: %s <Game Boy executable file name>\n", argv[0]);
        exit(-1);
    }

    FILE* fin;
    fin = fopen(argv[1], "rb");
    fread(rom, 1, MAX_ROM_SIZE, fin);
    fclose(fin);

    Decoder decoder(rom);

    /**
     * TODO: change while loop into clock cycling
     */
    int cycles = 0;
    while(cycles < 10) {
        Inst next_inst = decoder.next();
        d_print_inst(next_inst); // debugging
        cycles++;
    }

    return 0;
}