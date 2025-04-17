#include <iostream>

#include "debug.h"
#include "decoder.h"

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("usage: %s <Game Boy executable file name>\n", argv[0]);
        exit(-1);
    }

    Decoder decoder(argv[1]);

    /**
     * TODO: change while loop into clock cycling
     */
    int cycles = 0;
    while(cycles < 10) {
        Inst next_inst = decoder.next();
        d_print_inst(next_inst);
        cycles++;
    }

    return 0;
}