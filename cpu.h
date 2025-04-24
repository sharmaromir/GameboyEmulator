#pragma once

#include <vector>
#include <functional>
#include <cstdint>

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
    Register() {};
    Register(WORD val) : word(val) {};
};

class CPU {

public:
    CPU() {};
    CPU(BYTE* rom);

    BYTE read_mem(WORD addr);
    void write_mem(WORD addr, BYTE data);
    
    void interrupt(int signal);
    int check_interrupts();
    void handle_interrupt(int signal);
    void update_timers(int cycles);

    void key_pressed(int key_code);
    void key_released(int key_code);

    BYTE screen[144][160][3];
    BYTE IME = 0; // interrupt master enable
    BYTE IME_next = 0;

    // execute the next instruction returns the number of cycles the instruction took
    uint32_t exec();

    BYTE ram[RAM_BANK_SIZE];
    Register af, bc, de, hl, sp, pc;
    uint32_t cycles;
    BYTE* rom;
    BYTE curr_rom_bank, curr_ram_bank;
    bool mbc1, mbc2, mbc3;
    bool rom_banking;
    bool ram_en;
    int divider_reg;
    int timer_counter;
    BYTE joypad_state;
    int clock_speed;
    BYTE halted = 0;
    BYTE stopped = 0;
    WORD rom_bank_count = 0;
    BYTE* rom_clone;

    void bank_mem(WORD addr, BYTE data);
    void set_clock_freq();
    inline void write_r8(BYTE r, BYTE data);
    inline BYTE read_r8(BYTE r);
    inline void write_r16(BYTE r, WORD data);
    inline WORD read_r16(BYTE r);
    inline void write_r16mem(BYTE r, WORD data);
    inline WORD read_r16mem(BYTE r);
    inline void write_r16stk(BYTE r, WORD data, bool high);
    inline WORD read_r16stk(BYTE r, bool high);
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