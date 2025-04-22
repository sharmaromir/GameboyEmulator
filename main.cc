#include <iostream>
#include <chrono>
#include <thread>

#include "cpu.h"

#define MAX_ROM_SIZE 0x200000
#define CYCLES_PER_FRAME 10
#define FPS 60

// int main(int argc, char** argv) {
//     if (argc != 2) {
//         printf("usage: %s <Game Boy executable file name>\n", argv[0]);
//         exit(-1);
//     }
//     printf("loading rom\n");

//     BYTE rom[MAX_ROM_SIZE];

//     FILE* fin;
//     fin = fopen(argv[1], "rb");
//     fread(rom, 1, MAX_ROM_SIZE, fin);
//     fclose(fin);

//     CPU cpu(rom);

//     printf("starting\n");

//     uint32_t temp_clock_cap = 10;

//     uint32_t curr_cycles = 0;
//     std::chrono::milliseconds frame_dur(1000 / FPS);
//     while(temp_clock_cap-- > 0) {
//         auto frameStart = std::chrono::steady_clock::now();
//         while(curr_cycles < CYCLES_PER_FRAME) {
//             curr_cycles += cpu.exec();
//         }
//         curr_cycles = 0;

//         auto frameEnd = std::chrono::steady_clock::now();
//         auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart);
//         if (elapsed < frame_dur) {
//             std::this_thread::sleep_for(frame_dur - elapsed);
//         }
//     }

//     return 0;
// }