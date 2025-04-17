#ifndef _MEM_H_
#define _MEM_H_

#include <cstdint>

// This is the memory interface
// We assume that memory is zero-filled


// Write the given byte value at the given address
extern void mem_write8(uint64_t address, uint8_t data);

extern void mem_write32(uint64_t address, uint32_t data);

extern void mem_write64(uint64_t address, uint64_t data);

extern uint8_t mem_read8(uint64_t addr);

extern uint32_t mem_read32(uint64_t addr);

extern uint64_t mem_read64(uint64_t addr);

#endif
