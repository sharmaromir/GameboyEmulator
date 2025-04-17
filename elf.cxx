#include "elf.h"
#include "mem.h"
#include <fcntl.h>
#include <cstdlib>
#include <cstdio>
#include <sys/mman.h>
#include <sys/stat.h>

struct elf_header {
    uint8_t ignore_1[24];
    uint64_t entry;
    uint64_t prog_header_offset;
    uint8_t ignore_2[14];
    uint16_t program_header_entry_size;
    uint16_t program_header_entry_count;
};

struct program_header {
    uint32_t type;
    uint32_t flags;
    uint64_t offset;
    uint64_t vaddr;
    uint64_t paddr;
    uint64_t filesz;
    uint64_t memsz;
};

uint64_t entry;
uint16_t entry_size;
uint16_t entry_count;
uint64_t firstAddr;

uint64_t loadElf(char const * const fileName) {
    int fd = open(fileName,O_RDONLY);
    if (fd < 0) {
        perror(fileName);
        exit(-1);
    }
    
    struct stat statBuffer;
    int rc = fstat(fd,&statBuffer);
    if (rc != 0) {
        perror("stat");
        exit(-1);
    }
    
    uintptr_t ptr = (uintptr_t) mmap(0, statBuffer.st_size, PROT_READ, MAP_FILE | MAP_PRIVATE, fd, 0);
    if ((void*)ptr == MAP_FAILED) {
        perror("mmap");
        exit(-1);
    }
    
    
    
    elf_header *header = (elf_header*) ptr;
    entry = header->entry;
    entry_size = header->program_header_entry_size;
    entry_count = header->program_header_entry_count;
    
    auto progHeader = (program_header*)(ptr + header->prog_header_offset);
    bool set = false;
    for (uint32_t i=0; i<entry_count; i++) {
        if (progHeader->type == 1) {
            uint8_t *dataPtr = reinterpret_cast<uint8_t*>(ptr + progHeader->offset);
            uint64_t vaddr = progHeader->vaddr;
            if(!set){
                set = true;
                firstAddr = vaddr;
            }
            uint64_t filesz = progHeader->filesz;
            for (uint64_t i=0; i<filesz; i++) {
                auto addr = vaddr + i;
                auto byte = dataPtr[i];
                mem_write8(addr, byte);
            }
            
            
        }
        
        progHeader = reinterpret_cast<program_header*>(((uintptr_t) progHeader) + entry_size);
    }
    
    return entry;
}
