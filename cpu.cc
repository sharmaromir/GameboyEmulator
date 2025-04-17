#include <iostream>
#include <vector>
#include <functional>
#include <memory>

#include "cpu.h"

CPU::CPU(BYTE* rom) : pc(PC_START), rom(rom), af(0x01B0), bc(0x0013), de(0x00D8), hl(0x014D), sp(0xFFFE), cycles(0) {
    r8 = {
        [this]() -> BYTE& { return B; },
        [this]() -> BYTE& { return C; },
        [this]() -> BYTE& { return D; },
        [this]() -> BYTE& { return E; },
        [this]() -> BYTE& { return H; },
        [this]() -> BYTE& { return L; },
        [this, rom]() -> BYTE& { return rom[HL]; },  // dynamically resolved
        [this]() -> BYTE& { return A; }
    };

    r16 = {
        [this]() -> WORD& { return BC; },
        [this]() -> WORD& { return DE; },
        [this]() -> WORD& { return HL; },
        [this]() -> WORD& { return SP; }
    };

    r16stk = {
        [this]() -> Register& { return bc; },
        [this]() -> Register& { return de; },
        [this]() -> Register& { return hl; },
        [this]() -> Register& { return af; }
    };
    
    r16mem = {
        [this]() -> WORD& { return BC; },
        [this]() -> WORD& { return DE; },
        [this]() -> WORD& { return HL; }, // increment
        [this]() -> WORD& { return HL; } // decrement
    };
}

inline void CPU::ld_r_r(BYTE r1, BYTE r2) {
    cycles = 1 + (r2 == 6 || r1 == 6);
    r8[r1]() = r8[r2]();
    PC += 1;
}

inline void CPU::ld_r_imm(BYTE r) {
    cycles = 2 + (r == 6);
    r8[r]() = rom[PC + 1];
    PC += 2;
}

inline void CPU::ld_a_r16mem(BYTE r) {
    cycles = 2;
    A = rom[r16mem[r]()];
    if(r == 2) {
        HL++;
    } else if (r == 3) {
        HL--;
    }
    PC += 1;
}

inline void CPU::ld_r16mem_a(BYTE r) {
    cycles = 2;
    rom[r16mem[r]()] = A;
    if(r == 2) {
        HL++;
    } else if (r == 3) {
        HL--;
    }
    PC += 1;
}

inline void CPU::ld_rr_nn(BYTE r) {
    cycles = 3;
    r16[r]() = rom[PC + 1] + (rom[PC + 2] << 8);
    PC += 3;
}

inline void CPU::push_rr(BYTE r) {
    cycles = 4;
    rom[--SP] = (r16stk[r]()).high;
    rom[--SP] = (r16stk[r]()).low;
    PC += 1;
}

inline void CPU::pop_rr(BYTE r) {
    cycles = 3;
    (r16stk[r]()).low = rom[SP++];
    (r16stk[r]()).high = rom[SP++];
    PC += 1;
}

inline void CPU::add_r(BYTE carry, BYTE r) {
    cycles = 1 + (r == 6);
    A += r8[r]() + (carry ? (F&0b00010000)>>4 : 0);
    F &= 0x0F;
    if(A == 0) F |= 0b10000000; // set zero flag
    if(A & 0b00001000) F |= 0b00100000; // set half carry flag
    if(A & 0b10000000) F |= 0b00010000; // set carry flag
    PC += 1;
}

inline void CPU::sub_r(BYTE carry, BYTE r) {
    cycles = 1 + (r == 6);
    A -= r8[r]() + (carry ? (F&0b00010000)>>4 : 0);
    F &= 0x0F;
    if(A == 0) F |= 0b10000000; // set zero flag
    F |= 0b01000000; // set subtract flag
    if(A & 0b00001000) F |= 0b00100000; // set half carry flag
    if(A & 0b10000000) F |= 0b00010000; // set carry flag
    PC += 1;
}

inline void CPU::cp_r(BYTE r) {
    cycles = 1 + (r == 6);
    F &= 0x0F;
    if(A == r8[r]()) F |= 0b10000000; // set zero flag
    if(((A - r8[r]()) & 0b00001000)) F |= 0b00100000; // set half carry flag
    if(A < r8[r]()) F |= 0b00010000; // set carry flag
    PC += 1;
}

inline void CPU::inc_r(BYTE r) {
    cycles = 1 + (r == 6)*2;
    r8[r]()++;
    F &= 0x1F;
    if(r8[r]() == 0) F |= 0b10000000; // set zero flag
    if(r8[r]() & 0b00001000) F |= 0b00100000; // set half carry flag
    PC += 1;
}

inline void CPU::dec_r(BYTE r) {
    cycles = 1 + (r == 6)*2;
    r8[r]()--;
    F &= 0x1F;
    if(r8[r]() == 0) F |= 0b10000000; // set zero flag
    if(r8[r]() & 0b00001000) F |= 0b00100000; // set half carry flag
    PC += 1;
}

inline void CPU::bit_and(BYTE r) {
    cycles = 1 + (r == 6);
    A &= r8[r]();
    F &= 0x1F;
    if(A == 0) F |= 0b10000000; // set zero flag
    F |= 0b00100000; // set half carry flag
    PC += 1;
}

inline void CPU::bit_or(BYTE r) {
    cycles = 1 + (r == 6);
    A |= r8[r]();
    F &= 0x1F;
    if(A == 0) F |= 0b10000000; // set zero flag
    PC += 1;
}

inline void CPU::bit_xor(BYTE r) {
    cycles = 1 + (r == 6);
    A ^= r8[r]();
    F &= 0x1F;
    if(A == 0) F |= 0b10000000; // set zero flag
    PC += 1;
}

inline void CPU::inc_r16(BYTE r) {
    cycles = 2;
    r16[r]()++;
    PC += 1;
}

inline void CPU::dec_r16(BYTE r) {
    cycles = 2;
    r16[r]()--;
    PC += 1;
}

inline void CPU::add_r16(BYTE r) {
    cycles = 2;
    HL += r16[r]();
    F &= 0x8F; // clear flags except zero
    if(Half & 0b10000000) {
        F |= 0b00010000; // set carry flag
    }
    if(Half & 0b00001000){
        F |= 0b00100000; // set half carry flag
    }
    PC += 1;
}

inline void CPU::rotate_left_circular(BYTE r) {
    cycles = 2 + 2*(r==6);
    F &= 0x0F; // clear flags
    F |= (r8[r]() & 0b10000000) >> 3; // set carry flag
    r8[r]() = (r8[r]() << 1) | (r8[r]() >> 7);
    F |= (r8[r]() == 0) << 7; // set zero flag
    PC += 1;
}

inline void CPU::rotate_right_circular(BYTE r) {
    cycles = 2 + 2*(r==6);
    F &= 0x0F; // clear flags
    F |= (r8[r]() & 0b00000001) << 4; // set carry flag
    r8[r]() = (r8[r]() >> 1) | (r8[r]() << 7);
    F |= (r8[r]() == 0) << 7; // set zero flag
    PC += 1;
}

inline void CPU::rotate_left(BYTE r) {
    cycles = 2 + 2*(r==6);
    BYTE carry = Carry;
    F &= 0x0F; // clear flags
    F |= (r8[r]() & 0b10000000) >> 3; // set carry flag
    r8[r]() = (r8[r]() << 1) | (carry);
    F |= (r8[r]() == 0) << 7; // set zero flag
    PC += 1;
}

inline void CPU::rotate_right(BYTE r) {
    cycles = 2 + 2*(r==6);
    BYTE carry = Carry;
    F &= 0x0F; // clear flags
    F |= (r8[r]() & 0b00000001) << 4; // set carry flag
    r8[r]() = (r8[r]() >> 1) | (carry << 7);
    F |= (r8[r]() == 0) << 7; // set zero flag
    PC += 1;
}

inline void CPU::sla(BYTE r) {
    cycles = 2 + 2*(r==6);
    F &= 0x0F; // clear flags
    F |= (r8[r]() & 0b10000000) >> 3; // set carry flag
    r8[r]() <<= 1;
    F |= (r8[r]() == 0) << 7; // set zero flag
    PC += 1;
}

inline void CPU::sra(BYTE r) {
    cycles = 2 + 2*(r==6);
    F &= 0x0F; // clear flags
    F |= (r8[r]() & 0b00000001) << 4; // set carry flag
    r8[r]() = (r8[r]() >> 1) | (r8[r]() & 0b10000000);
    F |= (r8[r]() == 0) << 7; // set zero flag
    PC += 1;
}

inline void CPU::srl(BYTE r) {
    cycles = 2 + 2*(r==6);
    F &= 0x0F; // clear flags
    F |= (r8[r]() & 0b00000001) << 4; // set carry flag
    r8[r]() = (r8[r]() >> 1);
    F |= (r8[r]() == 0) << 7; // set zero flag
    PC += 1;
}

inline void CPU::swap(BYTE r) {
    cycles = 2 + 2*(r==6);
    F &= 0x0F; // clear flags
    r8[r]() = ((r8[r]() & 0b00001111) << 4) | ((r8[r]() & 0b11110000) >> 4);
    F |= (r8[r]() == 0) << 7; // set zero flag
    PC += 1;
}

inline bool CPU::test_flag(int flag) {
    return ((F & (1 << flag))) ? true : false;
    // ZNHC0000
}

inline void CPU::jump(bool is_conditional, int flag, int condition) {
    WORD nn = rom[PC + 1] + (rom[PC + 2] << 8);
    if (!is_conditional || flag == condition) {
        PC = nn;
        cycles = 4;
    } else {
        PC += 3;
        cycles = 3;     // duration on pg111
    }
}

inline void CPU::jump_relative(bool is_conditional, int flag, int condition) {
    SIGNED_BYTE n = rom[PC + 1];
    if (!is_conditional || flag == condition) {
        PC += n;
        cycles = 3;
    } else {
        PC += 2;
        cycles = 2;    // pg 114
    }
}

inline void CPU::call(bool is_conditional, int flag, int condition) {
    WORD nn = rom[PC + 1] + (rom[PC + 2] << 8);
    if (!is_conditional || flag == condition) {
        rom[--SP] = pchigh;
        rom[--SP] = pclow;
        PC = nn;
        cycles = 6;
    } else {
        PC += 2;
        cycles = 3;
    }
}

inline void CPU::ret(bool is_conditional, int flag, int condition) {
    if (!is_conditional) {
        cycles = 4;
        PC = rom[SP++] + (rom[SP++] << 8);
    } else if (flag == condition) {
        cycles = 5;
        PC = rom[SP++] + (rom[SP++] << 8);
    } else {
        cycles = 2;
        PC += 1;
    }
}

inline void CPU::restart(BYTE n) {
    cycles = 4;
    rom[--SP] = pchigh;
    rom[--SP] = pclow;
    PC = n;
}

uint32_t CPU::exec() {
    BYTE opc = rom[PC];

    printf("THE OPC: %X ", opc);
    switch(opc) {
        // nop
        case 0x00:
            // d_missing(opc);
            printf("NOP\n");
            cycles = 1;
            PC += 1;
            break;
        case 0b01000000:
            ld_r_r(0,0);
            break;
        case 0b01000001:
            ld_r_r(0,1);  
            break;
        case 0b01000010:
            ld_r_r(0,2);            
            break;
        case 0b01000011:
            ld_r_r(0,3);            
            break;
        case 0b01000100:
            ld_r_r(0,4);            
            break;
        case 0b01000101:
            ld_r_r(0,5);            
            break;
        case 0b01000110:
            ld_r_r(0,6);            
            break;
        case 0b01000111:
            ld_r_r(0,7);            
            break;
        case 0b01001000:
            ld_r_r(1,0);            
            break;
        case 0b01001001:
            ld_r_r(1,1);           
            break;
        case 0b01001010:
            ld_r_r(1,2);            
            break;
        case 0b01001011:
            ld_r_r(1,3);
            break;
        case 0b01001100:
            ld_r_r(1,4);
            break;
        case 0b01001101:
            ld_r_r(1,5);
            break;
        case 0b01001110:
            ld_r_r(1,6);
            break;
        case 0b01001111:
            ld_r_r(1,7);
            break;
        case 0b01010000:
            ld_r_r(2,0);
            break;
        case 0b01010001:
            ld_r_r(2,1);
            break;
        case 0b01010010:
            ld_r_r(2,2);
            break;
        case 0b01010011:
            ld_r_r(2,3);
            break;
        case 0b01010100:
            ld_r_r(2,4);
            break;
        case 0b01010101:
            ld_r_r(2,5);
            break;
        case 0b01010110:
            ld_r_r(2,6);
            break;
        case 0b01010111:
            ld_r_r(2,7);
            break;
        case 0b01011000:
            ld_r_r(3,0);
            break;
        case 0b01011001:
            ld_r_r(3,1);
            break;
        case 0b01011010:
            ld_r_r(3,2);
            break;
        case 0b01011011:
            ld_r_r(3,3);
            break;
        case 0b01011100:
            ld_r_r(3,4);
            break;
        case 0b01011101:
            ld_r_r(3,5);
            break;
        case 0b01011110:
            ld_r_r(3,6);
            break;
        case 0b01011111:
            ld_r_r(3,7);
            break;
        case 0b01100000:
            ld_r_r(4,0);
            break;
        case 0b01100001:
            ld_r_r(4,1);
            break;
        case 0b01100010:
            ld_r_r(4,2);
            break;
        case 0b01100011:    
            ld_r_r(4,3);
            break;
        case 0b01100100:
            ld_r_r(4,4);
            break;
        case 0b01100101:
            ld_r_r(4,5);
            break;
        case 0b01100110:
            ld_r_r(4,6);
            break;
        case 0b01100111:
            ld_r_r(4,7);
            break;
        case 0b01101000:    
            ld_r_r(5,0);
            break;
        case 0b01101001:
            ld_r_r(5,1);
            break;
        case 0b01101010:
            ld_r_r(5,2);
            break;
        case 0b01101011:    
            ld_r_r(5,3);
            break;
        case 0b01101100:
            ld_r_r(5,4);
            break;
        case 0b01101101:
            ld_r_r(5,5);
            break;
        case 0b01101110:
            ld_r_r(5,6);
            break;
        case 0b01101111:
            ld_r_r(5,7);
            break;
        case 0b01110000:
            ld_r_r(6,0);
            break;
        case 0b01110001:
            ld_r_r(6,1);
            break;
        case 0b01110010:
            ld_r_r(6,2);
            break;
        case 0b01110011:
            ld_r_r(6,3);
            break;
        case 0b01110100:
            ld_r_r(6,4);
            break;
        case 0b01110101:
            ld_r_r(6,5);
            break;
        case 0b01110110:
            ld_r_r(6,6);
            break;
        case 0b01110111:
            ld_r_r(6,7);
            break;
        case 0b01111000:
            ld_r_r(7,0);
            break;
        case 0b01111001:
            ld_r_r(7,1);
            break;
        case 0b01111010:
            ld_r_r(7,2);
            break;
        case 0b01111011:
            ld_r_r(7,3);
            break;
        case 0b01111100:
            ld_r_r(7,4);
            break;
        case 0b01111101:
            ld_r_r(7,5);
            break;
        case 0b01111110:
            ld_r_r(7,6);
            break;
        case 0b01111111:
            ld_r_r(7,7);
        // load register (immediate)
            break;
        case 0b00000110:
            ld_r_imm(0);
            break;
        case 0b00001110:
            ld_r_imm(1);
            break;
        case 0b00010110:
            ld_r_imm(2);
            break;
        case 0b00011110:
            ld_r_imm(3);
            break;
        case 0b00100110:
            ld_r_imm(4);
            break;
        case 0b00101110:
            ld_r_imm(5);
            break;
        case 0b00110110:
            ld_r_imm(6);
            break;
        case 0b00111110:
            ld_r_imm(7);
            break;
        // load accumulator (indirect register)
        case 0b00001010:
            ld_a_r16mem(0);
            break;
        case 0b00011010:
            ld_a_r16mem(1);
            break;
        case 0b00101010:
            ld_a_r16mem(2);
            break;
        case 0b00111010:
            ld_a_r16mem(3);
            break;
        // load from accumulator (indirect register)
        case 0b00000010:
            ld_r16mem_a(0);
            break;
        case 0b00010010:
            ld_r16mem_a(1);
            break;
        case 0b00100010:
            ld_r16mem_a(2);
            break;
        case 0b00110010:
            ld_r16mem_a(3);
            break;
        // load accumulator direct
        case 0xFA:
            cycles = 4;
            A = rom[rom[PC + 1] + (rom[PC + 2] << 8)];
            PC += 3;
            break;
        // load from accumulator direct
        case 0xEA:
            cycles = 4;
            rom[rom[PC + 1] + (rom[PC + 2] << 8)] = A;
            PC += 3;
            break;
        // load accumulator indirect c
        case 0xF2:
            cycles = 2;
            A = rom[0xFF00 + C];
            PC += 1;
            break;
        // load from accumulator indirect c
        case 0xE2:
            cycles = 2;
            rom[0xFF00 + C] = A;
            PC += 1;
            break;
        // load accumulator direct 
        case 0xF0:
            cycles = 3;
            A = rom[0xFF00 + rom[PC + 1]];
            PC += 2;
            break;
        // load from accumulator direct
        case 0xE0:
            cycles = 3;
            rom[0xFF00 + rom[PC + 1]] = A;
            PC += 2;
            break;
        // load 16 bit register
        case 0b00000001:
            ld_rr_nn(0);
            break;
        case 0b00010001:
            ld_rr_nn(1);
            break;
        case 0b00100001:
            ld_rr_nn(2);
            break;
        case 0b00110001:
            ld_rr_nn(3);
            break;
        // load from stack pointer (direct)
        case 0x08:
            cycles = 5;
            rom[rom[PC + 1] + (rom[PC + 2] << 8)] = SP;
            PC += 3;
            break;
        // load stack pointer from hl
        case 0xF9:
            cycles = 2;
            SP = HL;
            PC += 1;
            break;
        // push register to stack
        case 0b11'00'0101:
            push_rr(0);
            break;
        case 0b11'01'0101:
            push_rr(1);
            break;
        case 0b11'10'0101:
            push_rr(2);
            break;
        case 0b11'11'0101:
            push_rr(3);
            break;
        // pop register from stack
        case 0b11'00'0001:
            pop_rr(0);
            break;
        case 0b11'01'0001:
            pop_rr(1);
            break;
        case 0b11'10'0001:
            pop_rr(2);
            break;
        case 0b11'11'0001:
            pop_rr(3);
            break;
        // load hl from adjusted sp
        case 0xF8:
            cycles = 3;
            HL = SP + SIGNED_BYTE(rom[PC + 1]);
            F &= 0x0F; // clear flags
            if(HL & 0b10000000) {
                F |= 0b00010000; // set carry flag
            }
            if(HL & 0b00001000){
                F |= 0b00100000; // set half carry flag
            }
            PC += 2;
            break;
        // add register to accumulator
        case 0b10000000:
            add_r(0, 0);
            break;
        case 0b10000001:
            add_r(0, 1);
            break;
        case 0b10000010:
            add_r(0, 2);
            break;
        case 0b10000011:
            add_r(0, 3);
            break;
        case 0b10000100:
            add_r(0, 4);
            break;
        case 0b10000101:
            add_r(0, 5);
            break;
        case 0b10000110:
            add_r(0, 6);
            break;
        case 0b10000111:
            add_r(0, 7);
            break;
        // add immediate to accumulator
        case 0xC6:
            cycles = 2;
            A += rom[PC + 1];
            F &= 0x0F;
            if(A == 0) F |= 0b10000000; // set zero flag
            if(A & 0b00001000) F |= 0b00100000; // set half carry flag
            if(A & 0b10000000) F |= 0b00010000; // set carry flag
            PC += 2;
            break;
        // add with carry
        case 0b10001000:
            add_r(1, 0);
            break;
        case 0b10001001:
            add_r(1, 1);
            break;
        case 0b10001010:
            add_r(1, 2);
            break;
        case 0b10001011:
            add_r(1, 3);
            break;
        case 0b10001100:
            add_r(1, 4);
            break;
        case 0b10001101:
            add_r(1, 5);
            break;
        case 0b10001110:
            add_r(1, 6);
            break;
        case 0b10001111:
            add_r(1, 7);
            break;
        // add immediate with carry
        case 0xCE:
            cycles = 2;
            A += rom[PC + 1] + ((F & 0b00010000) >> 4);
            F &= 0x0F;
            if(A == 0) F |= 0b10000000; // set zero flag
            if(A & 0b00001000) F |= 0b00100000; // set half carry flag
            if(A & 0b10000000) F |= 0b00010000; // set carry flag
            PC += 2;
            break;
        // subtract register from accumulator
        case 0b10010000:
            sub_r(0, 0);
            break;
        case 0b10010001:
            sub_r(0, 1);
            break;
        case 0b10010010:
            sub_r(0, 2);
            break;
        case 0b10010011:
            sub_r(0, 3);
            break;
        case 0b10010100:
            sub_r(0, 4);
            break;
        case 0b10010101:
            sub_r(0, 5);
            break;
        case 0b10010110:
            sub_r(0, 6);
            break;
        case 0b10010111:
            sub_r(0, 7);
            break;
        // subtract immediate from accumulator
        case 0xD6:
            cycles = 2;
            A -= rom[PC + 1];
            F &= 0x0F;
            if(A == 0) F |= 0b10000000; // set zero flag
            if(A & 0b00001000) F |= 0b00100000; // set half carry flag
            if(A & 0b10000000) F |= 0b00010000; // set carry flag
            PC += 2;
            break;
        // subtract with carry
        case 0b10011000:
            sub_r(1, 0);
            break;
        case 0b10011001:
            sub_r(1, 1);
            break;
        case 0b10011010:
            sub_r(1, 2);
            break;
        case 0b10011011:    
            sub_r(1, 3);
            break;
        case 0b10011100:
            sub_r(1, 4);
            break;
        case 0b10011101:
            sub_r(1, 5);
            break;
        case 0b10011110:
            sub_r(1, 6);
            break;
        case 0b10011111:
            sub_r(1, 7);
            break;
        // subtract immediate with carry
        case 0xDE:
            cycles = 2;
            A -= rom[PC + 1] + ((F & 0b00010000) >> 4);
            F &= 0x0F;
            if(A == 0) F |= 0b10000000; // set zero flag
            if(A & 0b00001000) F |= 0b00100000; // set half carry flag
            if(A & 0b10000000) F |= 0b00010000; // set carry flag
            PC += 2;
            break;
        // compare register
        case 0b10111000:
            cp_r(0);
            break;
        case 0b10111001:
            cp_r(1);
            break;
        case 0b10111010:
            cp_r(2);
            break;
        case 0b10111011:
            cp_r(3);
            break;
        case 0b10111100:
            cp_r(4);
            break;
        case 0b10111101:
            cp_r(5);
            break;
        case 0b10111110:
            cp_r(6);
            break;
        case 0b10111111:
            cp_r(7);
            break;
        // compare immediate
        case 0xFE:
            cycles = 2;
            F &= 0x0F;
            if(A == rom[PC + 1]) F |= 0b10000000; // set zero flag
            if(((A - rom[PC + 1]) & 0b00001000)) F |= 0b00100000; // set half carry flag
            if(A < rom[PC + 1]) F |= 0b00010000; // set carry flag
            PC += 2;
            break;
        // increment register
        case 0b00000100:
            inc_r(0);
            break;
        case 0b00001100:
            inc_r(1);
            break;
        case 0b00010100:
            inc_r(2);
            break;
        case 0b00011100:
            inc_r(3);
            break;
        case 0b00100100:
            inc_r(4);
            break;
        case 0b00101100:
            inc_r(5);
            break;
        case 0b00110100:
            inc_r(6);
            break;
        case 0b00111100:
            inc_r(7);
            break;
        // decrement register
        case 0b00000101:
            dec_r(0);
            break;
        case 0b00001101:
            dec_r(1);
            break;
        case 0b00010101:
            dec_r(2);
            break;
        case 0b00011101:
            dec_r(3);
            break;
        case 0b00100101:
            dec_r(4);
            break;
        case 0b00101101:
            dec_r(5);
            break;
        case 0b00110101:
            dec_r(6);
            break;
        case 0b00111101:
            dec_r(7);
            break;
        // bitwise and register
        case 0b10100000:
            bit_and(0);
            break;
        case 0b10100001:
            bit_and(1);
            break;
        case 0b10100010:
            bit_and(2);
            break;
        case 0b10100011:
            bit_and(3);
            break;
        case 0b10100100:
            bit_and(4);
            break;
        case 0b10100101:
            bit_and(5);
            break;
        case 0b10100110:
            bit_and(6);
            break;
        case 0b10100111:
            bit_and(7);
            break;
        // bitwise and immediate
        case 0xE6:
            cycles = 2;
            A &= rom[PC + 1];
            F &= 0x0F;
            if(A == 0) F |= 0b10000000; // set zero flag
            F |= 0b00100000; // set half carry flag
            PC += 2;
            break;
        // bitwise or register
        case 0b10110000:
            bit_or(0);
            break;
        case 0b10110001:
            bit_or(1);
            break;
        case 0b10110010:
            bit_or(2);
            break;
        case 0b10110011:
            bit_or(3);
            break;
        case 0b10110100:
            bit_or(4);
            break;
        case 0b10110101:
            bit_or(5);
            break;
        case 0b10110110:
            bit_or(6);
            break;
        case 0b10110111:
            bit_or(7);
            break;
        // bitwise or immediate
        case 0xF6:
            cycles = 2;
            A |= rom[PC + 1];
            F &= 0x0F;
            if(A == 0) F |= 0b10000000; // set zero flag
            PC += 2;
            break;
        // bitwise xor register
        case 0b10101000:
            bit_xor(0);
            break;
        case 0b10101001:
            bit_xor(1);
            break;
        case 0b10101010:
            bit_xor(2);
            break;
        case 0b10101011:
            bit_xor(3);
            break;
        case 0b10101100:
            bit_xor(4);
            break;
        case 0b10101101:
            bit_xor(5);
            break;
        case 0b10101110:
            bit_xor(6);
            break;
        case 0b10101111:
            bit_xor(7);
            break;
        // bitwise xor immediate
        case 0xEE:
            cycles = 2;
            A ^= rom[PC + 1];
            F &= 0x0F;
            if(A == 0) F |= 0b10000000; // set zero flag
            PC += 2;
            break;
        // complement carry flag
        case 0x3F:
            cycles = 1;
            F &= 0x9F;
            F ^= 0b00010000;
            PC += 1;
            break;
        // set carry flag
        case 0x37:
            cycles = 1;
            F &= 0x9F;
            F |= 0b00010000;
            PC += 1;
            break;
        // decimal adjustment  MAYBE WRONG*****
        case 0x27:{
            cycles = 1;
            BYTE offset = 0;
            if((N && (A & 0x0F) > 9) || Half) {
                offset |= 0x06;
            }
            if((N && (A & 0xF0) > 0x90) || Carry) {
                offset |= 0x60;
            }
            if(N) {
                A -= offset;
            } else {
                A += offset;
            }
            F &= 0x4F;
            if(A == 0) F |= 0b10000000; // set zero flag
            if(offset >= 0x60) F |= 0b00010000; // set carry flag
            PC += 1;
            break;
        }
        // complement accumulator
        case 0x2F:
            cycles = 1;
            A = ~A;
            F &= 0x9F;
            F |= 0b01100000; // set subtract and half carry flag
            PC += 1;
            break;
        // inc 16 bit register
        case 0b00000011:
            inc_r16(0);
            break;
        case 0b00010011:
            inc_r16(1);
            break;
        case 0b00100011:
            inc_r16(2);
            break;
        case 0b00110011:
            inc_r16(3);
            break;
        // dec 16 bit register
        case 0b00001011:
            dec_r16(0);
            break;
        case 0b00011011:
            dec_r16(1);
            break;
        case 0b00101011:
            dec_r16(2);
            break;
        case 0b00111011:
            dec_r16(3);
            break;
        // add 16 bit register to hl
        case 0b00001001:
            add_r16(0);
            break;
        case 0b00011001:
            add_r16(1);
            break;
        case 0b00101001:
            add_r16(2);
            break;
        case 0b00111001:
            add_r16(3);
            break;
        // add relative to stack pointer
        case 0xE8:
            cycles = 4;
            SP += SIGNED_BYTE(rom[PC + 1]);
            F &= 0x0F; // clear flags
            if(SP & 0b10000000) {
                F |= 0b00010000; // set carry flag
            }
            if(SP & 0b00001000){
                F |= 0b00100000; // set half carry flag
            }
            PC += 2;
            break;
        // rotate left circular
        case 0x07:
            cycles = 1;
            F &= 0x0F; // clear flags
            F |= (A & 0b10000000) >> 3; // set carry flag
            A = (A << 1) | (A >> 7);
            PC += 1;
            break;
        // rotate right circular
        case 0x0F:
            cycles = 1;
            F &= 0x0F; // clear flags
            F |= (A & 0b00000001) << 4; // set carry flag
            A = (A >> 1) | (A << 7);
            PC += 1;
            break;
        // rotate left through carry
        case 0x17:{
            cycles = 1;
            BYTE carry = Carry;
            F &= 0x0F; // clear flags
            F |= (A & 0b10000000) >> 3; // set carry flag
            A = (A << 1) | (C);
            PC += 1;
            break;
        }
        // rotate right through carry
        case 0x1F:{
            cycles = 1;
            BYTE carry = Carry;
            F &= 0x0F; // clear flags
            F |= (A & 0b00000001) << 4; // set carry flag
            A = (A >> 1) | (C << 7);
            PC += 1;
            break;
        }
        case 0xCB:{
            PC++;
            BYTE opcode = rom[PC]; 
            switch(opcode){
                case 0b00000000:
                    rotate_left_circular(0);
                    break;
                case 0b00000001:
                    rotate_left_circular(1);
                    break;
                case 0b00000010:
                    rotate_left_circular(2);
                    break;
                case 0b00000011:
                    rotate_left_circular(3);
                    break;
                case 0b00000100:
                    rotate_left_circular(4);
                    break;
                case 0b00000101:
                    rotate_left_circular(5);
                    break;
                case 0b00000110:
                    rotate_left_circular(6);
                    break;
                case 0b00000111:
                    rotate_left_circular(7);
                    break;
                case 0b00001000:
                    rotate_right_circular(0);
                    break;
                case 0b00001001:
                    rotate_right_circular(1);
                    break;
                case 0b00001010:
                    rotate_right_circular(2);
                    break;
                case 0b00001011:
                    rotate_right_circular(3);
                    break;
                case 0b00001100:
                    rotate_right_circular(4);
                    break;
                case 0b00001101:
                    rotate_right_circular(5);
                    break;
                case 0b00001110:
                    rotate_right_circular(6);
                    break;
                case 0b00001111:
                    rotate_right_circular(7);
                    break;
                case 0b00010000:
                    rotate_left(0);
                    break;
                case 0b00010001:
                    rotate_left(1);
                    break;
                case 0b00010010:
                    rotate_left(2);
                    break;
                case 0b00010011:    
                    rotate_left(3);
                    break;
                case 0b00010100:
                    rotate_left(4);
                    break;
                case 0b00010101:
                    rotate_left(5);
                    break;
                case 0b00010110:
                    rotate_left(6);
                    break;
                case 0b00010111:
                    rotate_left(7);
                    break;
                case 0b00011000:
                    rotate_right(0);
                    break;
                case 0b00011001:
                    rotate_right(1);
                    break;
                case 0b00011010:
                    rotate_right(2);
                    break;
                case 0b00011011:
                    rotate_right(3);
                    break;
                case 0b00011100:
                    rotate_right(4);
                    break;
                case 0b00011101:
                    rotate_right(5);
                    break;
                case 0b00011110:
                    rotate_right(6);
                    break;
                case 0b00011111:
                    rotate_right(7);
                    break;
                case 0b00100000:
                    sla(0);
                    break;
                case 0b00100001:
                    sla(1);
                    break;
                case 0b00100010:
                    sla(2);
                    break;
                case 0b00100011:
                    sla(3);
                    break;
                case 0b00100100:
                    sla(4);
                    break;
                case 0b00100101:
                    sla(5);
                    break;
                case 0b00100110:
                    sla(6);
                    break;
                case 0b00100111:
                    sla(7);
                    break;
                case 0b00101000:
                    sra(0);
                    break;
                case 0b00101001:
                    sra(1);
                    break;
                case 0b00101010:
                    sra(2);
                    break;
                case 0b00101011:
                    sra(3);
                    break;
                case 0b00101100:
                    sra(4);
                    break;
                case 0b00101101:
                    sra(5);
                    break;
                case 0b00101110:
                    sra(6);
                    break;
                case 0b00101111:
                    sra(7);
                    break;
                // swapping nibbles
                case 0b00110000:
                    swap(0);
                    break;
                case 0b00110001:
                    swap(1);
                    break;
                case 0b00110010:
                    swap(2);
                    break;
                case 0b00110011:
                    swap(3);
                    break;
                case 0b00110100:
                    swap(4);
                    break;
                case 0b00110101:
                    swap(5);
                    break;
                case 0b00110110:
                    swap(6);
                    break;
                case 0b00110111:
                    swap(7);
                    break;
                // shift right logical
                case 0b00111000:
                    srl(0);
                    break;
                case 0b00111001:
                    srl(1);
                    break;
                case 0b00111010:
                    srl(2);
                    break;
                case 0b00111011:
                    srl(3);
                    break;
                case 0b00111100:
                    srl(4);
                    break;
                case 0b00111101:
                    srl(5);
                    break;
                case 0b00111110:
                    srl(6);
                    break;
                case 0b00111111:
                    srl(7);
                    break;
                default:
                    switch(opcode>>6){
                        case 0b00:
                            std::cerr << "cb meow?" << std::endl;exit(-1);
                        case 0b01: // test bit
                            cycles = 2 + (opcode&0b00000111==6);
                            F &= 0x1F;
                            F |= 0b00100000; // set half carry flag
                            if(((r8[opcode & 0b00000111]())>>((opcode>>3)&7))&1 == 0) F |= 0b10000000; // set zero flag
                            break;
                        case 0b10: // reset bit
                            cycles = 2 + (opcode&0b00000111==6);
                            r8[opcode & 0b00000111]() &= ~(1 << ((opcode>>3)&7));
                            break;
                        case 0b11: // set bit
                            cycles = 2 + (opcode&0b00000111==6);
                            r8[opcode & 0b00000111]() |= (1 << ((opcode>>3)&7));
                            break;  
                    }
            }
            break;
        }
        // jumps to immediate  
        case 0xC3:  // unconditional
            jump(false, 0, false);
            break;
        case 0xE9:  // jump hl
            cycles = 4;
            PC = HL;
            break;
        case 0b11000010:    // cc = 00, NZ
            jump(true, Z, false);
            break;
        case 0b11001010:    // cc = 01, Z
            jump(true, Z, true);
            break;
        case 0b11010010:    // cc = 10, NC
            jump(true, Carry, false);
            break;
        case 0b11011010:    // cc = 11, C
            jump(true, Carry, true);
            break;
        // jumps to relative address
        case 0x18:  // unconditional
            jump_relative(false, 0, false);
            break;
        case 0b00100000:    // cc = 00, NZ
            jump_relative(true, Z, false);
            break;
        case 0b00101000:    // cc = 01, Z
            jump_relative(true, Z, true);
            break;
        case 0b00110000:    // cc = 10, NC
            jump_relative(true, Carry, false);
            break;
        case 0b00111000:    // cc = 11, C
            jump_relative(true, Carry, true);
            break;
        // calls
        case 0xCD:  // unconditional
            call(false, 0, false);
            break;
        case 0b11000100:    // cc = 00, NZ
            call(true, Z, false);
            break;
        case 0b11001100:    // cc = 01, Z
            call(true, Z, true);
            break;
        case 0b11010100:    // cc = 10, NC
            call(true, Carry, false);
            break;
        case 0b11011100:    // cc = 11, C
            call(true, Carry, true);
            break;
        // returns
        case 0xC9: // unconditional
            ret(false, 0, false);
            break;
        case 0b11000000:    // cc = 00, NZ
            ret(true, Z, false);
            break;
        case 0b11001000:    // cc = 01, Z
            ret(true, Z, true);
            break;
        case 0b11010000:    // cc = 10, NC
            ret(true, Carry, false);
            break;
        case 0b11011000:    // cc = 11, C
            ret(true, Carry, true);
            break;
        // return from interrupt handler
        case 0xD9:
            cycles = 4;  
            PC = rom[SP++] + (rom[SP++] << 8);
            IME = 1;
            break;
        // restarts
        case 0b11000111:    // rst 0x0
            restart(0x00);
            break;
        case 0b11001111:    // rst 0x08
            restart(0x08);
            break;
        case 0b11010111:    // rst 0x10
            restart(0x10);
            break;
        case 0b11011111:    // rst 0x18
            restart(0x18);
            break;
        case 0b11100111:    // rst 0x20
            restart(0x20);
            break;
        case 0b11101111:    // rst 0x28
            restart(0x28);
            break;
        case 0b11110111:    // rst 0x30
            restart(0x30);
            break;
        case 0b11111111:    // rst 0x38
            restart(0x38);
            break;
        // disable/enable interrupts
        case 0xF3:
            IME = 0;
            PC += 1;
            cycles = 1;
            break;
        case 0xFB:
            IME_next = 1;
            PC += 1;
            cycles = 1;
            break;
        default:
            std::cerr << "meow?" << std::endl;exit(-1);
            break;
    }

    uint32_t ret = cycles;
    cycles = 0;
    return ret;
}