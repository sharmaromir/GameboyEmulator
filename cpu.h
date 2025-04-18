#pragma once

#include <vector>
#include <functional>

#define PC_START 0x100
#define RAM_BANK_SIZE 0x8000

typedef unsigned char BYTE;
typedef char SIGNED_BYTE;
typedef unsigned short WORD;
typedef signed short SIGNED_WORD;

#define Z (af.low>>7)&1
#define N (af.low>>6)&1
#define Half (af.low>>5)&1
#define Carry (af.low>>4)&1
#define A af.high
#define F af.low
#define B bc.high
#define C bc.low
#define D de.high
#define E de.low
#define H hl.high
#define L hl.low
#define AF af.word
#define BC bc.word
#define DE de.word
#define HL hl.word
#define SP sp.word
#define PC pc.word
#define splow sp.low
#define sphigh sp.high
#define pclow pc.low
#define pchigh pc.high

union Register {
    struct {
        BYTE low;
        BYTE high;
    };
    WORD word;

    Register(WORD val) : word(val) {};
};

class CPU {

public:
    CPU(BYTE* rom);

    BYTE read_mem(WORD addr);
    void write_mem(WORD addr, BYTE data);
    void interrupt(int signal);
    void checkInterrupts();
    void handleInterrupt(int signal);

    // execute the next instruction returns the number of cycles the instruction took
    uint32_t exec();

private:
    BYTE* rom;
    BYTE ram[RAM_BANK_SIZE];
    BYTE curr_rom_bank, curr_ram_bank;
    uint32_t cycles;
    Register af, bc, de, hl, sp, pc;
    std::vector<std::function<BYTE&()>> r8;
    std::vector<std::function<WORD&()>> r16;
    std::vector<std::function<Register&()>> r16stk;
    std::vector<std::function<WORD&()>> r16mem;
    BYTE IME; // interrupt master enable
    BYTE IME_next;
    bool mbc1, mbc2;
    bool ram_en, rom_banking;

    void bank_mem(WORD addr, BYTE data);

    inline void ld_r_r(BYTE r1, BYTE r2);
    inline void ld_r_imm(BYTE r);
    inline void ld_a_r16mem(BYTE r);
    inline void ld_r16mem_a(BYTE r);
    inline void ld_rr_nn(BYTE r);
    inline void push_rr(BYTE r);
    inline void pop_rr(BYTE r);
    inline void add_r(BYTE carry, BYTE r);
    inline void sub_r(BYTE carry, BYTE r);
    inline void cp_r(BYTE r);
    inline void inc_r(BYTE r);
    inline void dec_r(BYTE r);
    inline void bit_and(BYTE r);
    inline void bit_or(BYTE r);
    inline void bit_xor(BYTE r);
    inline void inc_r16(BYTE r);
    inline void dec_r16(BYTE r);
    inline void add_r16(BYTE r);
    inline void rotate_left_circular(BYTE r);
    inline void rotate_right_circular(BYTE r);
    inline void rotate_left(BYTE r);
    inline void rotate_right(BYTE r);
    inline void sla(BYTE r);
    inline void sra(BYTE r);
    inline void srl(BYTE r);
    inline void swap(BYTE r);
    inline bool test_flag(int flag);
    inline void jump(bool is_conditional, int flag, int condition);
    inline void jump_relative(bool is_conditional, int flag, int condition);
    inline void call(bool is_conditional, int flag, int condition);
    inline void ret(bool is_conditional, int flag, int condition);
    inline void restart(BYTE n);
};