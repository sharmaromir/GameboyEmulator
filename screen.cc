
#include <SDL/SDL.h>
#include <SDL/SDL_opengl.h>
#include <iostream>
#include <chrono>
#include <thread>
#include "lcd.h"
#include <stdlib.h>
#include <string>
#include "cpu.h"
#include "ppu.h"

static const int windowWidth = 160*2;
static const int windowHeight = 144*2;

#define CYCLES_PER_FRAME 69905
#define FPS 60

#define VERTICAL_BLANK_SCAN_LINE 0x90
#define VERTICAL_BLANK_SCAN_LINE_MAX 0x99
#define RETRACE_START 456
#define MAX_ROM_SIZE 0x200000

typedef std::string string;

CPU cpu;
LCD lcd;
PPU ppu;
int cycles_this_update = 0;

void init_screen() {
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("Failed to initialize SDL: %s\n", SDL_GetError());
        exit(1);
    }

    SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);

    if (SDL_SetVideoMode(windowWidth, windowHeight, 32, SDL_OPENGL) == NULL) {
        printf("Failed to set video mode: %s\n", SDL_GetError());
        exit(1);
    }

    glViewport(0, 0, windowWidth, windowHeight);
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glOrtho(0, windowWidth, windowHeight, 0, -1.0, 1.0);

    SDL_WM_SetCaption("Gameboy Emulator", NULL);

    glClearColor(0, 0, 0, 1.0);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glShadeModel(GL_FLAT);

    glDisable(GL_TEXTURE_2D);
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
	glPixelZoom(2, -2);
 	glDrawPixels(160, 144, GL_RGB, GL_UNSIGNED_BYTE, cpu.screen); // SCREEN DATA GOES HERE
	SDL_GL_SwapBuffers( ) ;
}

void emulator_update() {
    int cycle_cnt = 0;
    while (cycle_cnt < CYCLES_PER_FRAME) {
 		int curr_cycles = cpu.exec();
        lcd.update(cpu, ppu, curr_cycles);
        cpu.check_interrupts();
        cpu.update_timers(curr_cycles);
        cycle_cnt += curr_cycles;
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
            printf("Error reading file\n");
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
    load_rom("tests/cpu_instrs/individual/01-special.gb");
}

int get_key(int code) {
    switch(code) {
        case SDLK_RIGHT:
            return 0;
            break;
        case SDLK_LEFT:
            return 1;
            break;
        case SDLK_UP:
            return 2;
            break;
        case SDLK_DOWN:
            return 3;
            break;
        case SDLK_a:
            return 4;
            break;
        case SDLK_s:
            return 5;
            break;
        case SDLK_SPACE:
            return 6;
            break;
        case SDLK_RETURN:
            return 7;
            break;
        default:
            return -1; // unmapped key
            break;
    }
}

void game_loop() {
    bool quit = false;
    SDL_Event event;
    
    std::chrono::milliseconds frame_dur(1000 / FPS);
    while(!quit) {
        while(SDL_PollEvent(&event)) {
            int key_code;
            switch(event.type) {
                case SDL_QUIT:
                    quit = true;
                    break;
                case SDL_KEYDOWN:
                    key_code = get_key(event.key.keysym.sym);
                    if(key_code >= 0)
                        cpu.key_pressed(key_code);
                    break;
                case SDL_KEYUP:
                    key_code = get_key(event.key.keysym.sym);
                    if(key_code >= 0)
                        cpu.key_released(key_code);
                    break;
                default:
                    break;
            }
        }
        
        if(quit) break;

        auto frameStart = std::chrono::steady_clock::now();
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
