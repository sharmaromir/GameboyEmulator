#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include <iostream>
#include <chrono>
#include <thread>
#include "emulator.h"

static const int windowWidth = 160;
static const int windowHeight = 144;

#define CYCLES_PER_FRAME 10
#define FPS 60

#define VERTICAL_BLANK_SCAN_LINE 0x90
#define VERTICAL_BLANK_SCAN_LINE_MAX 0x99
#define RETRACE_START 456
#define MAX_ROM_SIZE 0x200000

CPU cpu;
LCD lcd;
PPU ppu;
int cycles_this_update = 0;

void init_screen() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        fprintf(stderr, "Failed to initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    if (SDL_SetVideoMode(windowWidth, windowHeight, 32, SDL_OPENGL) == NULL) {
        fprintf(stderr, "Failed to set video mode: %s\n", SDL_GetError());
        exit(1);
    }

    glViewport(0, 0, windowWidth, windowHeight);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glOrtho(0, windowWidth, windowHeight, 0, -1.0, 1.0);

    glClearColor(0, 0, 0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glShadeModel(GL_FLAT);

    glEnable(GL_TEXTURE_2D);
    glDisable(GL_DEPTH_TEST);
    glDisable(GL_CULL_FACE);
    glDisable(GL_DITHER);
    glDisable(GL_BLEND);

    SDL_GL_SwapBuffers();
}

void render_game() {
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
 	glLoadIdentity();
 	glRasterPos2i(-1, 1);
	glPixelZoom(1, -1);
 	glDrawPixels(160, 144, GL_RGB, GL_UNSIGNED_BYTE, cpu.screen); // SCREEN DATA GOES HERE
	SDL_GL_SwapBuffers( ) ;
}


void emulator_update() {
    cycles_this_update = 0 ;
	const int target_cycles = 70221 ;
    while (cycles_this_update < target_cycles) {
 		int cycles = cpu.exec() ;
 		cycles_this_update += cycles;
        lcd.update(cpu, ppu, cycles);
    }
    render_game();
}


bool load_rom(const string& rom_name) {
    BYTE rom[MAX_ROM_SIZE];

    FILE* fin;
    fin = fopen(rom_name.c_str(), "rb");
    size_t read_count = fread(rom, 1, MAX_ROM_SIZE, fin);
    if (read_count != MAX_ROM_SIZE) {
        if (ferror(fin)) {
            fprintf(stderr, "Error reading file\n");
            exit(1);
        }
    }
    fclose(fin);

    cpu = CPU(rom);
    lcd = LCD();
    ppu = PPU();

    return true;
}

void emulator_setup() {
    load_rom("tests/cpu_instrs/individual/04-op r,imm.gb");
}


void game_loop() {
    uint32_t temp_clock_cap = 100;

    uint32_t curr_cycles = 0;
    
    std::chrono::milliseconds frame_dur(1000 / FPS);
    while(temp_clock_cap-- > 0) {
        auto frameStart = std::chrono::steady_clock::now();
        while(curr_cycles < CYCLES_PER_FRAME) {
            curr_cycles += cpu.exec();
        }
        curr_cycles = 0;
        emulator_update();
        auto frameEnd = std::chrono::steady_clock::now();
        auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(frameEnd - frameStart);
        if (elapsed < frame_dur) {
            std::this_thread::sleep_for(frame_dur - elapsed);
        }
    }
}

int main(int argc, char** argv) {
    
    init_screen();
    emulator_setup();
    game_loop();
    // SDL_Delay(3000);
    
    SDL_Quit( ) ;
    return 0;
}
