#include <iostream>
#include <vector>
#include <functional>
#include <memory>
#include <cstring>

#include "cpu.h"

CPU::CPU(BYTE* rom) : af(0x01B0), bc(0x0013), de(0x00D8), hl(0x014D), sp(0xFFFE), pc(PC_START), 
                      cycles(0), rom(rom), curr_rom_bank(1), curr_ram_bank(0), 
                      mbc1(false), mbc2(false), rom_banking(true), divider_reg(0), timer_counter(1024), joypad_state(0xFF) {
    // initialize rom values
    rom[0xFF05] = 0x00;
    rom[0xFF06] = 0x00;
    rom[0xFF07] = 0x00;
    rom[0xFF10] = 0x80;
    rom[0xFF11] = 0xBF;
    rom[0xFF12] = 0xF3;
    rom[0xFF14] = 0xBF;
    rom[0xFF16] = 0x3F;
    rom[0xFF17] = 0x00;
    rom[0xFF19] = 0xBF;
    rom[0xFF1A] = 0x7F;
    rom[0xFF1B] = 0xFF;
    rom[0xFF1C] = 0x9F;
    rom[0xFF1E] = 0xBF;
    rom[0xFF20] = 0xFF;
    rom[0xFF21] = 0x00;
    rom[0xFF22] = 0x00;
    rom[0xFF23] = 0xBF;
    rom[0xFF24] = 0x77;
    rom[0xFF25] = 0xF3;
    rom[0xFF26] = 0xF1;
    rom[0xFF40] = 0x91;
    rom[0xFF42] = 0x00;
    rom[0xFF43] = 0x00;
    rom[0xFF45] = 0x00;
    rom[0xFF47] = 0xFC;
    rom[0xFF48] = 0xFF;
    rom[0xFF49] = 0xFF;
    rom[0xFF4A] = 0x00;
    rom[0xFF4B] = 0x00;
    rom[0xFFFF] = 0x00;

    // set mbc type
    if(rom[0x147] == 1 || rom[0x147] == 2 || rom[0x147] == 3) {
        mbc1 = true;
    }else if(rom[0x147] == 5 || rom[0x147] == 6) {
        mbc2 = true;
    }

    // set ram banks to 0
    memset(ram, 0, sizeof(ram));

}

BYTE CPU::read_r8(BYTE r) {
    switch(r) {
        case 0: return B;
        case 1: return C;
        case 2: return D;
        case 3: return E;
        case 4: return H;
        case 5: return L;
        case 6: return read_mem(HL);
        case 7: return A;
    }
    exit(-1);
}

void CPU::write_r8(BYTE r, BYTE data) {
    switch(r) {
        case 0: B = data; break;
        case 1: C = data; break;
        case 2: D = data; break;
        case 3: E = data; break;
        case 4: H = data; break;
        case 5: L = data; break;
        case 6: write_mem(HL, data); break;
        case 7: A = data; break;
    }
}

void CPU::write_r16(BYTE r, WORD data) {
    switch(r) {
        case 0: bc.word = data; break;
        case 1: de.word = data; break;
        case 2: hl.word = data; break;
        case 3: sp.word = data; break;
    }
}

WORD CPU::read_r16(BYTE r) {
    switch(r) {
        case 0: return bc.word;
        case 1: return de.word;
        case 2: return hl.word;
        case 3: return sp.word;
    }
    exit(-1);
}

void CPU::write_r16mem(BYTE r, WORD data) {
    exit(0);
}

WORD CPU::read_r16mem(BYTE r) {
    switch(r) {
        case 0: return (BC);
        case 1: return (DE);
        case 2: return (HL);
        case 3: return (HL);
    }
    exit(-1);
}

void CPU::write_r16stk(BYTE r, WORD data, bool high) {
    switch(r) {
        case 0: if(high){bc.high = data;}else{bc.low = data;} break;
        case 1: if(high){de.high = data;}else{de.low = data;} break;
        case 2: if(high){hl.high = data;}else{hl.low = data;} break;
        case 3: if(high){af.high = data;}else{af.low = (data&0xF0);} break;
    }
}

WORD CPU::read_r16stk(BYTE r, bool high) {
    switch(r) {
        case 0: if(high){return bc.high;}else{return bc.low;}
        case 1: if(high){return de.high;}else{return de.low;}
        case 2: if(high){return hl.high;}else{return hl.low;}
        case 3: if(high){return af.high;}else{return af.low;}
    }
    exit(-1);
}

void CPU::set_clock_freq() {
    BYTE freq = read_mem(0xFF07) & 3;
    switch(freq) {
        case 0: timer_counter = 1024; break; // freq 4096
        case 1: timer_counter = 16; break; // freq 262144
        case 2: timer_counter = 64; break; // freq 65536
        case 3: timer_counter = 256; break; // freq 16382
    }
}


BYTE CPU::read_mem(WORD addr) {
    // rom bank
    if((addr >= 0x4000) && (addr <= 0x7FFF)) {
        return rom[(addr - 0x4000) + (curr_rom_bank * 0x4000)];
    }
    // ram bank
    else if((addr >= 0xA000) && (addr <= 0xBFFF)) {
        return ram[(addr - 0xA000) + (curr_ram_bank * 0x2000)];
    }
    // input
    else if(addr == 0xFF00) {
        BYTE ret = ~(rom[0xFF00]);
        // standard buttons
        if(!(ret & (1 << 4))) {
            ret &= (joypad_state >> 4) | 0xF0; 
        }
        // directional buttons
        else if(!(ret & (1 << 5))) {
            ret &= (joypad_state & 0xF) | 0xF0;
        }
        return ret;
    }
    // memory
    else {
        return rom[addr];
    }
}

void CPU::write_mem(WORD addr, BYTE data) {
    // banking
    if(addr < 0x8000) {
        bank_mem(addr, data);
    }
    // write to ram
    else if ((addr >= 0xA000) && (addr < 0xC000) && ram_en) {
        if(ram_en) {
            ram[(addr - 0xA000) + (curr_ram_bank*0x2000)] = data;
        }
    }
    // echo ram
    else if((addr >= 0xE000) && (addr < 0xFE00)) {
        rom[addr] = data;
        write_mem(addr-0x2000, data);
    }
    // timer controller
    else if(addr == 0xFF07) {
        rom[addr] = data;
        set_clock_freq();
    }
    // divider register
    else if(addr == 0xFF04) {
        rom[0xFF04] = 0;
    }
    // write if not restrictied address
    else if (!((addr >= 0xFEA0) && (addr < 0xFEFF))) {
        rom[addr] = data;
    }
}

void CPU::bank_mem(WORD addr, BYTE data) {
    // ram enable
    if(addr < 0x2000 && (mbc1 || mbc2)) {
        if (mbc2 && addr & (1<<4)) {
            return;
        }

        if ((data & 0xF) == 0xA)
            ram_en = true;
        else if ((data & 0xF) == 0x0)
            ram_en = false;
    }
    // rom bank change
    else if((addr >= 0x2000) && (addr < 0x4000) && (mbc1 || mbc2)) {
        if(mbc2) {
            curr_rom_bank = data & 0xF;
            
            if(curr_rom_bank == 0)
                curr_rom_bank++;
        }else {
            if(data == 0) {
                data = 1;
            }
            curr_rom_bank = (curr_rom_bank & 0xE0) | (data & 0x1F);
            if (curr_rom_bank == 0)
                curr_rom_bank++;
        }
    }
    // do ROM or RAM bank change
    else if((addr >= 0x4000) && (addr < 0x6000) && mbc1) {
        if(rom_banking) {
            curr_ram_bank = 0;
            data = (data & 3) << 5;
            if(curr_rom_bank == 0)
                data++;
            curr_rom_bank = (curr_rom_bank & 0x1F) | data;
            if(curr_rom_bank == 0)
                curr_rom_bank++;
        }else {
            curr_ram_bank = data & 3;
        }
    }
    // change rom ram mode
    else if((addr >= 0x6000) && (addr < 0x8000) && mbc1) {
        rom_banking = !(data & 0x1);
        if (rom_banking)
            curr_ram_bank = 0;
    }
}

void CPU::handle_interrupt(int signal) {
    IME = false;
    write_mem(0xFF0F, read_mem(0xFF0F) & ~(1 << signal)); // reset bit
    // push pc onto stack
    write_mem(--SP, pchigh);
    write_mem(--SP, pclow);
    switch (signal) {
        case 0:
            PC = 0x40;
            break;
        case 1:
            PC = 0x48;
            break;
        case 2:
            PC = 0x50;
            break;
        case 3:
            PC = 0x60;
            break;
        default:
            fprintf(stderr, "Invalid interrupt signal");
            exit(1);
    }
}

void CPU::interrupt(int signal) {
    write_mem(0xFF0F, read_mem(0xFF0F) | (1 << signal)); // set bit
}

void CPU::check_interrupts() {
    BYTE IRR = read_mem(0xFF0F); // Interupt Request Register
    BYTE IER = read_mem(0xFFFF); // Interupt Enabled Register
    if (IME && IRR > 0) { // master interupt switch
        for (int i = 0; i < 5; i++) {
            if ((IRR & (1 << i)) && (IER & (1 << i))) {
                handle_interrupt(i);
            }
        }
    }
}

void CPU::update_timers(int cycles) {
    // update divider register
    if(255 - divider_reg < cycles)
        rom[0xFF04]++;
    divider_reg += cycles;

    // update timers if clock is enabled
    if(read_mem(0xFF07) & (1 << 2)) {
        timer_counter -= cycles;
        if(timer_counter <= 0) {
            set_clock_freq();

            // timer overflow
            BYTE timer = read_mem(0xFF05);
            if(timer == 255) {
                write_mem(0xFF05, read_mem(0xFF06));
                handle_interrupt(2);
            }else {
                write_mem(0xFF05, timer+1);
            }
        }
    }
}

void CPU::key_pressed(int key_code) {
    bool already_pressed = !(joypad_state & (1 << key_code));
    bool std_btn = key_code > 3;
    
    // set new joypad state
    joypad_state = ~((~joypad_state) | (1 << key_code));

    BYTE key_req = rom[0xFF00];
    bool req_interrupt = (std_btn && !(key_req & (1 << 5))) || (!std_btn && !(key_req & (1 << 4)));

    if(req_interrupt && !already_pressed) {
        interrupt(4);
    }
}

void CPU::key_released(int key_code) {
    joypad_state = joypad_state & (~(1 << key_code));
}

inline void CPU::ld_r_r(BYTE r1, BYTE r2) {
    cycles = 1 + (r2 == 6 || r1 == 6);
    write_r8(r1, read_r8(r2)); 
    PC += 1;
}

inline void CPU::ld_r_imm(BYTE r) {
    cycles = 2 + (r == 6);
    write_r8(r, read_mem(PC + 1));
    PC += 2;
}

inline void CPU::ld_a_r16mem(BYTE r) {
    cycles = 2;
    A = read_mem(read_r16mem(r));
    if(r == 2) {
        HL++;
    } else if (r == 3) {
        HL--;
    }
    PC += 1;
}

inline void CPU::ld_r16mem_a(BYTE r) {
    cycles = 2;
    write_mem(read_r16mem(r), A);
    if(r == 2) {
        HL++;
    } else if (r == 3) {
        HL--;
    }
    PC += 1;
}

inline void CPU::ld_rr_nn(BYTE r) {
    cycles = 3;
    write_r16(r, read_mem(PC+1) + (read_mem(PC+2) << 8));
    PC += 3;
}

inline void CPU::push_rr(BYTE r) {
    cycles = 4;
    write_mem(--SP, (read_r16stk(r, true)));
    write_mem(--SP, (read_r16stk(r, false)));
    PC += 1;
}

inline void CPU::pop_rr(BYTE r) {
    cycles = 3;
    write_r16stk(r, read_mem(SP++), false);
    write_r16stk(r, read_mem(SP++), true);
    PC += 1;
}

inline void CPU::add_r(BYTE carry, BYTE r) {
    cycles = 1 + (r == 6);
    BYTE saved_carry = Carry;
    F &= 0x0F;
    if(((A&0xF)+(read_r8(r)&0xF)+(carry ? saved_carry : 0))>>4) F |= 0b00100000; // set half carry flag
    if((A + read_r8(r) + (carry ? saved_carry : 0))>>8) F |= 0b00010000; // set carry flag
    A += read_r8(r) + (carry ? saved_carry : 0);
    if(A == 0) F |= 0b10000000; // set zero flag
    PC += 1;
}

inline void CPU::sub_r(BYTE carry, BYTE r) {
    cycles = 1 + (r == 6);
    BYTE saved_carry = Carry;
    F &= 0x0F;
    if((A&0xF) < ((read_r8(r)&0xF) + (carry ? saved_carry : 0))) F |= 0b00100000; // set half carry flag
    if(A < read_r8(r) + (carry ? saved_carry : 0)) F |= 0b00010000; // set carry flag
    A -= read_r8(r) + (carry ? saved_carry : 0);
    if(A == 0) F |= 0b10000000; // set zero flag
    F |= 0b01000000; // set subtract flag
    PC += 1;
}

inline void CPU::cp_r(BYTE r) {
    cycles = 1 + (r == 6);
    F &= 0x0F;
    BYTE value = read_r8(r);
    if(A == value) F |= 0b10000000; // set zero flag
    if((A&0xF) < (read_r8(r)&0xF)) F |= 0b00100000; // set half carry flag
    if(A < read_r8(r)) F |= 0b00010000; // set carry flag
    F |= 0b01000000; // set subtract flag
    PC += 1;
}

inline void CPU::inc_r(BYTE r) {
    cycles = 1 + (r == 6)*2;
    write_r8(r, read_r8(r)+1);
    F &= 0x1F;
    BYTE value = read_r8(r);
    if(value == 0) F |= 0b10000000; // set zero flag
    if((value & 0xF) == 0) F |= 0b00100000; // set half carry flag
    PC += 1;
}

inline void CPU::dec_r(BYTE r) {
    cycles = 1 + (r == 6)*2;
    write_r8(r, read_r8(r)-1);
    F &= 0x1F;
    BYTE value = read_r8(r);
    if(value == 0) F |= 0b10000000; // set zero flag
    if((value & 0xF) == 0xF) F |= 0b00100000; // set half carry flag
    F |= 0b01000000; // set subtract flag
    PC += 1;
}

inline void CPU::bit_and(BYTE r) {
    cycles = 1 + (r == 6);
    A &= read_r8(r);
    F &= 0x0F;
    if(A == 0) F |= 0b10000000; // set zero flag
    F |= 0b00100000; // set half carry flag
    PC += 1;
}

inline void CPU::bit_or(BYTE r) {
    cycles = 1 + (r == 6);
    A |= read_r8(r);
    F &= 0x0F;
    if(A == 0) F |= 0b10000000; // set zero flag
    PC += 1;
}

inline void CPU::bit_xor(BYTE r) {
    cycles = 1 + (r == 6);
    A ^= read_r8(r);
    F &= 0x0F;
    if(A == 0) F |= 0b10000000; // set zero flag
    PC += 1;
}

inline void CPU::inc_r16(BYTE r) {
    cycles = 2;
    write_r16(r, read_r16(r)+1);
    PC += 1;
}

inline void CPU::dec_r16(BYTE r) {
    cycles = 2;
    write_r16(r, read_r16(r)-1);
    PC += 1;
}

inline void CPU::add_r16(BYTE r) {
    cycles = 2;
    F &= 0x8F; // clear flags except zero
    if((HL + read_r16(r))>>16) {
        F |= 0b00010000; // set carry flag
    }
    if(((HL&0xFFF) + (read_r16(r)&0xFFF))>>12){
        F |= 0b00100000; // set half carry flag
    }
    HL += read_r16(r);
    PC += 1;
}

inline void CPU::rotate_left_circular(BYTE r) {
    cycles = 2 + 2*(r==6);
    F &= 0x0F; // clear flags
    BYTE value = read_r8(r);
    F |= (value & 0b10000000) >> 3; // set carry flag
    value = (value << 1) | (value >> 7);
    write_r8(r, value);
    F |= (value == 0) << 7; // set zero flag
    PC += 1;
}

inline void CPU::rotate_right_circular(BYTE r) {
    cycles = 2 + 2*(r==6);
    F &= 0x0F; // clear flags
    BYTE value = read_r8(r);
    F |= (value & 0b00000001) << 4; // set carry flag
    value = (value >> 1) | (value << 7);
    write_r8(r, value);
    F |= (value == 0) << 7; // set zero flag
    PC += 1;
}

inline void CPU::rotate_left(BYTE r) {
    cycles = 2 + 2*(r==6);
    BYTE carry = Carry;
    F &= 0x0F; // clear flags
    BYTE value = read_r8(r);
    F |= (value & 0b10000000) >> 3; // set carry flag
    value = (value << 1) | (carry);
    write_r8(r, value);
    F |= (value == 0) << 7; // set zero flag
    PC += 1;
}

inline void CPU::rotate_right(BYTE r) {
    cycles = 2 + 2*(r==6);
    BYTE carry = Carry;
    F &= 0x0F; // clear flags
    BYTE value = read_r8(r);
    F |= (value & 0b00000001) << 4; // set carry flag
    value = (value >> 1) | (carry << 7);
    write_r8(r, value);
    F |= (read_r8(r) == 0) << 7; // set zero flag
    PC += 1;
}

inline void CPU::sla(BYTE r) {
    cycles = 2 + 2*(r==6);
    F &= 0x0F; // clear flags
    BYTE value = read_r8(r);
    F |= (value & 0b10000000) >> 3; // set carry flag
    write_r8(r, value << 1);
    F |= (read_r8(r) == 0) << 7; // set zero flag
    PC += 1;
}

inline void CPU::sra(BYTE r) {
    cycles = 2 + 2*(r==6);
    F &= 0x0F; // clear flags
    BYTE value = read_r8(r);
    F |= (value & 0b00000001) << 4; // set carry flag
    write_r8(r, (value >> 1) | (value & 0b10000000));
    F |= (read_r8(r) == 0) << 7; // set zero flag
    PC += 1;
}

inline void CPU::srl(BYTE r) {
    cycles = 2 + 2*(r==6);
    F &= 0x0F; // clear flags
    BYTE value = read_r8(r);
    F |= (value & 0b00000001) << 4; // set carry flag
    write_r8(r, (value >> 1));
    F |= (read_r8(r) == 0) << 7; // set zero flag
    PC += 1;
}

inline void CPU::swap(BYTE r) {
    cycles = 2 + 2*(r==6);
    F &= 0x0F; // clear flags
    BYTE value = read_r8(r);
    write_r8(r, ((value & 0b11110000) >> 4) | ((value & 0b00001111) << 4));
    F |= (read_r8(r) == 0) << 7; // set zero flag
    PC += 1;
}

inline bool CPU::test_flag(int flag) {
    return ((F & (1 << flag))) ? true : false;
    // ZNHC0000
}

inline void CPU::jump(bool is_conditional, int flag, int condition) {
    WORD nn = read_mem(PC + 1) + (read_mem(PC + 2) << 8);
    if (!is_conditional || flag == condition) {
        PC = nn;
        cycles = 4;
    } else {
        PC += 3;
        cycles = 3;     // duration on pg111
    }
}

inline void CPU::jump_relative(bool is_conditional, int flag, int condition) {
    SIGNED_BYTE n = read_mem(PC + 1);
    PC += 2;
    if (!is_conditional || flag == condition) {
        PC += n;
        cycles = 3;
    } else {
        cycles = 2;    // pg 114
    }
}

inline void CPU::call(bool is_conditional, int flag, int condition) {
    WORD nn = read_mem(PC + 1) + (read_mem(PC + 2) << 8);
    if (!is_conditional || flag == condition) {
        PC += 3;
        write_mem(--SP, pchigh);
        write_mem(--SP, pclow);
        PC = nn;
        cycles = 6;
    } else {
        PC += 3;
        cycles = 3;
    }
}

inline void CPU::ret(bool is_conditional, int flag, int condition) {
    if (!is_conditional) {
        cycles = 4;
        BYTE low = read_mem(SP++);
        BYTE high = read_mem(SP++);
        PC = low + (high << 8);
    } else if (flag == condition) {
        cycles = 5;
        BYTE low = read_mem(SP++);
        BYTE high = read_mem(SP++);
        PC = low + (high << 8);
    } else {
        cycles = 2;
        PC += 1;
    }
}

inline void CPU::restart(BYTE n) {
    cycles = 4;
    PC++;
    write_mem(--SP, pchigh);
    write_mem(--SP, pclow);
    PC = n;
}

uint32_t CPU::exec() {
    BYTE opc = read_mem(PC);
    // WORD pcval = PC;
    // BYTE rmpc =  opc;
    // BYTE rmpc1 = read_mem(PC + 1);
    // BYTE rmpc2 = read_mem(PC + 2);
    // BYTE rmpc3 = read_mem(PC + 3);
    // fprintf(stderr, "A:%02X F:%02X B:%02X C:%02X D:%02X E:%02X H:%02X L:%02X SP:%04X PC:%04X PCMEM:%02X,%02X,%02X,%02X\n", A, F, B, C, D, E, H, L, SP, pcval, rmpc, rmpc1, rmpc2, rmpc3);
    switch(opc) {
        // nop
        case 0x00:
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
            A = read_mem(read_mem(PC + 1) + (read_mem(PC + 2) << 8));
            PC += 3;
            break;
        // load from accumulator direct
        case 0xEA:
            cycles = 4;
            write_mem(read_mem(PC + 1) + (read_mem(PC + 2) << 8), A);
            PC += 3;
            break;
        // load accumulator indirect c
        case 0xF2:
            cycles = 2;
            A = read_mem(0xFF00 + C);
            PC += 1;
            break;
        // load from accumulator indirect c
        case 0xE2:
            cycles = 2;
            write_mem(0xFF00 + C, A);
            PC += 1;
            break;
        // load accumulator direct 
        case 0xF0:
            cycles = 3;
            A = read_mem(0xFF00 + read_mem(PC + 1));
            PC += 2;
            break;
        // load from accumulator direct
        case 0xE0:
            cycles = 3;
            write_mem(0xFF00 + read_mem(PC + 1), A);
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
            write_mem(read_mem(PC + 1) + (read_mem(PC + 2) << 8), splow);
            write_mem(read_mem(PC + 1) + (read_mem(PC + 2) << 8) + 1, sphigh);
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
        case 0b11000001:
            pop_rr(0);
            break;
        case 0b11010001:
            pop_rr(1);
            break;
        case 0b11100001:
            pop_rr(2);
            break;
        case 0b11110001:
            pop_rr(3);
            break;
        // load hl from adjusted sp
        case 0xF8:
            cycles = 3;
            HL = SP + SIGNED_BYTE(read_mem(PC + 1));
            F &= 0x0F; // clear flags
            if((((SP&0xFF) + (SIGNED_BYTE(read_mem(PC + 1))&0xFF)))>>8) {
                F |= 0b00010000; // set carry flag
            }
            if(((SP&0xF) + (SIGNED_BYTE(read_mem(PC + 1))&0xF))>>4){
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
            F &= 0x0F;
            if(((A&0xF)+(read_mem(PC+1)&0xF))>>4) F |= 0b00100000; // set half carry flag
            if((A+read_mem(PC+1))>>8) F |= 0b00010000; // set carry flag
            A += read_mem(PC + 1);
            
            if(A == 0) F |= 0b10000000; // set zero flag
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
        case 0xCE:{
            cycles = 2;
            BYTE saved_carry = Carry;
            F &= 0x0F;
            if(((A&0xF)+(read_mem(PC+1)&0xF)+ (saved_carry))>>4) F |= 0b00100000; // set half carry flag
            if((A+read_mem(PC+1) +  (saved_carry))>>8) F |= 0b00010000; // set carry flag
            A += read_mem(PC + 1) + (saved_carry);
            if(A == 0) F |= 0b10000000; // set zero flag
            PC += 2;
            break;
        }
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
            F &= 0x0F;
            if((A&0xF) < (read_mem(PC+1)&0xF)) F |= 0b00100000; // set half carry flag
            if(A < read_mem(PC+1)) F |= 0b00010000; // set carry flag
            A -= read_mem(PC + 1);
            if(A == 0) F |= 0b10000000; // set zero flag
            F |= 0b01000000; // set subtract flag
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
        case 0xDE:{
            cycles = 2;
            BYTE saved_carry = Carry;
            F &= 0x0F;
            if((A&0xF) < ((read_mem(PC+1)&0xF) + (saved_carry))) F |= 0b00100000; // set half carry flag
            if(A < read_mem(PC+1) + (saved_carry)) F |= 0b00010000; // set carry flag
            A -= read_mem(PC + 1) + (saved_carry);
            if(A == 0) F |= 0b10000000; // set zero flag
            F |= 0b01000000; // set subtract flag
            PC += 2;
            break;
        }
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
        case 0xFE:{
            cycles = 2;
            F &= 0x0F;
            BYTE val = read_mem(PC + 1);
            if(A == val) F |= 0b10000000; // set zero flag
            if((A&0xF) < (val&0xF)) F |= 0b00100000; // set half carry flag
            if(A < val) F |= 0b00010000; // set carry flag
            F |= 0b01000000; // set subtract flag
            PC += 2;
            break;
        }
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
            A &= read_mem(PC + 1);
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
            A |= read_mem(PC + 1);
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
            A ^= read_mem(PC + 1);
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
            case 0x27: { // DAA - Decimal Adjust Accumulator
                cycles = 1;
                BYTE correction = 0;
                if (!(F & 0x40)) {
                    if ((F & 0x20) || (A & 0x0F) > 9) {
                        correction += 0x06;
                    }
                    if ((F & 0x10) || A > 0x99) {
                        correction += 0x60;
                        F |= 0x10; 
                    }
                    A += correction;
                }
                else {
                    if (F & 0x20) {
                        correction += 0x06;
                    }
                    if (F & 0x10) {
                        correction += 0x60;
                    }
                    A -= correction;
                }
                F &= ~0x20;
                if (A == 0) {
                    F |= 0x80; 
                } else {
                    F &= ~0x80; 
                }
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
            F &= 0x0F; // clear flags
            if(((SP&0xFF) + (SIGNED_BYTE(read_mem(PC + 1))&0xFF)) >> 8) {
                F |= 0b00010000; // set carry flag
            }
            if(((SP&0xF) + (SIGNED_BYTE(read_mem(PC + 1))&0xF)) >> 4) {
                F |= 0b00100000; // set half carry flag
            }
            SP += SIGNED_BYTE(read_mem(PC + 1));
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
            A = (A << 1) | (carry);
            PC += 1;
            break;
        }
        // rotate right through carry
        case 0x1F:{
            cycles = 1;
            BYTE carry = Carry;
            F &= 0x0F; // clear flags
            F |= (A & 0b00000001) << 4; // set carry flag
            A = (A >> 1) | (carry << 7);
            PC += 1;
            break;
        }
        case 0xCB:{
            PC++;
            BYTE opcode = read_mem(PC); 
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
                            cycles = 2 + ((opcode&0b00000111)==6);
                            F &= 0x1F;
                            F |= 0b00100000; // set half carry flag
                            if((((read_r8(opcode & 0b00000111))>>((opcode>>3)&7))&1) == 0) F |= 0b10000000; // set zero flag
                            PC += 1;
                            break;
                        case 0b10: // reset bit
                            cycles = 2 + ((opcode&0b00000111)==6);
                            write_r8(opcode & 0b00000111, read_r8(opcode & 0b00000111) & ~(1 << ((opcode>>3)&7)));
                            PC += 1;
                            break;
                        case 0b11: // set bit
                            cycles = 2 + ((opcode&0b00000111)==6);
                            write_r8(opcode & 0b00000111, read_r8(opcode & 0b00000111) | (1 << ((opcode>>3)&7)));
                            PC += 1;
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
        case 0xD9: {
            cycles = 4;  
            BYTE low = read_mem(SP++);
            BYTE high = read_mem(SP++);
            PC = low + (high << 8);
            IME = 1;
            break;
        }
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
            std::cerr << "meow?" << std::endl;
            fprintf(stderr, "Unknown opcode: 0x%02X\n", opc);
            exit(-1);
            break;
    }

    uint32_t ret = cycles;
    cycles = 0;
    return ret;
}