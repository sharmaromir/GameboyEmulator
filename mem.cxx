#include "mem.h"
#include "debug.h"
#include <stdlib.h>
#include <stdio.h>
#include <unordered_map>

typedef struct{
    uint64_t addr;
    uint8_t data;
    bool exists;
    bool collide;
}stored;

#define MEM_SIZE ((1ULL << 20) + 12345)

stored *memory = NULL;
bool collision = false;

std::unordered_map<uint64_t, uint8_t> slowMemory;


void mem_write8(uint64_t addr, uint8_t data) {
    if(memory == NULL) {
        memory = (stored*)calloc(MEM_SIZE, sizeof(stored));
    }
    if(addr == 0xFFFFFFFFFFFFFFFF){
        // fprintf(stderr, "printing!! %lx, %c\n", addr, static_cast<char>(data));
        printf("%c", static_cast<char>(data));
        return;
    }
    stored memValue = memory[addr%MEM_SIZE];
    if(!memValue.exists || memValue.addr == addr) {
        //fprintf(stderr, "writing %x to %lx\n", data, addr);
        memory[addr%MEM_SIZE] = {addr, data, true, memValue.collide};
    } else {
        slowMemory[addr] = data;
        collision = true;
        memory[addr%MEM_SIZE].collide = true;
    }
}

void mem_write32(uint64_t addr, uint32_t data) {
    mem_write8(addr, data & 0xFF);
    mem_write8(addr + 1, (data >> 8) & 0xFF);
    mem_write8(addr + 2, (data >> 16) & 0xFF);
    mem_write8(addr + 3, (data >> 24) & 0xFF);
}

void mem_write64(uint64_t addr, uint64_t data) {
    mem_write32(addr, data & 0xFFFFFFFF);
    mem_write32(addr + 4, data >> 32);
}

uint8_t mem_read8(uint64_t addr) {
    if(!collision) return memory[addr%MEM_SIZE].data;
    stored memValue = memory[addr%MEM_SIZE];
    if(memValue.addr == addr) {
        return memValue.data;
    }
    if(!collision) return 0;
    if(memValue.collide) {
        auto it = slowMemory.find(addr);
        if(it != slowMemory.end()) {
            return it->second;
        }
    }
    return 0;
}

uint32_t mem_read32(uint64_t addr) {
    return mem_read8(addr) |
     (mem_read8(addr + 1) << 8) |
      (mem_read8(addr + 2) << 16) |
       (mem_read8(addr + 3) << 24);
}

uint64_t mem_read64(uint64_t addr) {
    return mem_read32(addr) |
     (((uint64_t)mem_read32(addr + 4)) << 32);
}
