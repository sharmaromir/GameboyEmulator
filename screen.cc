
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

#define CYCLES_PER_FRAME 70221*2
#define FPS 59.73/2

#define VERTICAL_BLANK_SCAN_LINE 0x90
#define VERTICAL_BLANK_SCAN_LINE_MAX 0x99
#define RETRACE_START 456
#define MAX_ROM_SIZE 0x200000

typedef std::string string;

CPU cpu;
LCD lcd;
PPU ppu;
int cycles_this_update = 0;
bool saving;

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
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    glOrtho(0, 160, 144, 0, -1, 1);
    glMatrixMode(GL_MODELVIEW);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

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

int count = 0;
int minX;
int maxX;
int minY;
int maxY;
std::vector<BYTE> dirtyRect;

void render_game() {
    glLoadIdentity();
	glPixelZoom(2, -2);
    if (count < 2) {
        glRasterPos2i(0, 0);
        glDrawPixels(160, 144, GL_RGB, GL_UNSIGNED_BYTE, cpu.screen); // SCREEN DATA GOES HERE
        if (count) {
            minX = cpu.dirtyMinX;
            maxX = cpu.dirtyMaxX;
            minY = cpu.dirtyMinY;
            maxY = cpu.dirtyMaxY;
        }
        count++;
    } else {
        int tempMinX = std::min(minX, cpu.dirtyMinX);
        int tempMaxX = std::max(maxX, cpu.dirtyMaxX);
        int tempMinY = std::min(minY, cpu.dirtyMinY);
        int tempMaxY = std::max(maxY, cpu.dirtyMaxY);
        minX = cpu.dirtyMinX;
        maxX = cpu.dirtyMaxX;
        minY = cpu.dirtyMinY;
        maxY = cpu.dirtyMaxY;
        int width = tempMaxX - tempMinX + 1;
        int height = tempMaxY - tempMinY + 1;
        if (width > 0 && height > 0) {
            dirtyRect.resize(height * width * 3);
            for (int i = tempMinY; i <= tempMaxY; i++) {
                for (int j = tempMinX; j <= tempMaxX; j++) {
                    int idx = ((i - tempMinY) * width + (j - tempMinX)) * 3;
                    dirtyRect[idx] = cpu.screen[i][j][0];
                    dirtyRect[idx + 1] = cpu.screen[i][j][1];
                    dirtyRect[idx + 2] = cpu.screen[i][j][2];
                }
            }
            glRasterPos2i(tempMinX, tempMinY);
            glDrawPixels(width, height, GL_RGB, GL_UNSIGNED_BYTE, dirtyRect.data());
        }
    }
    SDL_GL_SwapBuffers();
    cpu.resetDirty();
}

void emulator_update() {
    int cycle_cnt = 0;
    while (cycle_cnt < CYCLES_PER_FRAME) {
        // BYTE rmpc = cpu.read_mem(cpu.PC);
        // BYTE rmpc1 = cpu.read_mem(cpu.PC + 1);
        // BYTE rmpc2 = cpu.read_mem(cpu.PC + 2);
        // BYTE rmpc3 = cpu.read_mem(cpu.PC + 3);
        // if(!cpu.halted) fprintf(stderr, "A:%02X F:%02X B:%02X C:%02X D:%02X E:%02X H:%02X L:%02X SP:%04X PC:%04X PCMEM:%02X,%02X,%02X,%02X\n", cpu.A, cpu.F, cpu.B, cpu.C, cpu.D, cpu.E, cpu.H, cpu.L, cpu.SP, cpu.PC, rmpc, rmpc1, rmpc2, rmpc3);
        int interrupt_cycles = cpu.check_interrupts();
        if(cpu.IME_next){
            cpu.IME = true;
            cpu.IME_next = false;
        }
        int curr_cycles = cpu.exec() + interrupt_cycles;
        lcd.update(cpu, ppu, curr_cycles);

        cpu.update_timers(curr_cycles);
        cycle_cnt += curr_cycles;
    }
    render_game();
}


bool load_rom(const string& rom_name) {
    BYTE rom[MAX_ROM_SIZE];
    saving = rom_name == "red.gb";
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
    if(saving){
        fin = fopen("red.sav", "rb");

        if (fin) {
            fread(cpu.ram, 1, 0x8000, fin);
            fclose(fin);
        } else {
            memset(cpu.ram, 0, 0x8000);
        }
    } 
    lcd = LCD();
    ppu = PPU();

    return true;
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
    
    std::chrono::milliseconds frame_dur((int)(1000.0 / FPS));
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
        // string temp;
        // std::getline(std::cin, temp);
    }
}

int main(int argc, char** argv) {
    if (argc < 2) {
        std::cerr << "No ROM name provided" << std::endl;
        return 1;
    }
    init_screen();
    load_rom(argv[1]);
    game_loop();
    // SDL_Delay(3000);
    if(saving){
        FILE* fout;
        fout = fopen("red.sav", "wb");
        if (fout) {
            fwrite(cpu.ram, 1, 0x8000, fout);
            fclose(fout);
        } else {
            printf("Error writing save file\n");
        }
    }
    SDL_Quit( ) ;
    return 0;
}
