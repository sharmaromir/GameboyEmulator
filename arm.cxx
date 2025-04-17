#include "elf.h"
#include "mem.h"
#include <stdio.h>
#include <stdlib.h>

extern uint64_t entry;
extern uint8_t *memory;
uint64_t registers[32];
uint64_t pc;
bool Z, N, C, V;

enum INSTRUCTION_TYPE {
    cbnz = 0,
    adrp = 1,
    addi = 2,
    ldr = 3,
    ldrb = 4,
    movz = 5,
    orr = 6,
    strb = 7,
    str = 8,
    ldp = 9,
    bl = 10,
    ret = 11,
    cnt = 12,
    sdiv = 13,
    subs = 14,
    csel = 15,
    err = 16
};

INSTRUCTION_TYPE getType(int x) {
    uint32_t bitMask;
    if((x & 0b1111111<<24) == 0b0110101<<24) {
        return cbnz;
    }
    if(((x & 0b1<<31) == 0b1<<31) && ((x & 0b11111<<24) == 0b10000<<24)) {
        return adrp;
    }

    if((x & 0b1<<31) == 0b1<<31){
        bitMask = x & 0b111111111<<21;
        if(bitMask == 0b111000010<<21 && (x & 0b1<<10) == 0b1<<10){
            return ldr;   
        }
        if(bitMask == 0b111000000<<21 && (x & 0b1<<10) == 0b1<<10){
            return str;
        }
        bitMask = x & 0b11111111<<22;
        if(bitMask == 0b11100101<<22){
            return ldr;
        }
        if(bitMask == 0b11100100<<22){
            return str;
        }
    }
    bitMask = x & 0b1111111111<<22;
    if(bitMask == 0b0011100101<<22) return ldrb;
    if(bitMask == 0b0011100100<<22) return strb;
    bitMask = x & 0b11111111111<<21;
    if(bitMask == 0b00111000010<<21 && (x & 0b1<<10) == 0b1<<10) {
        return ldrb;
    }
    if(bitMask == 0b00111000000<<21 && (x & 0b1<<10) == 0b1<<10) return strb;
    
    bitMask = x & 0b11111111<<23;
    if(bitMask == 0b100010<<23) return addi;
    if(bitMask == 0b10100101<<23) return movz;
    if(bitMask == 0b01100100<<23) return orr;
    if((x & 0b1111111111 << 21) == (0b0011010100 << 21) && (x & 0b11 << 10) == (0b0 << 10)){
        return csel;
    }
    if((x & 0b1111111111<<21) == (0b0011010110<<21) && ((x & 0b111111<<10) == 0b000011<<10)){
        return sdiv;
    }
    if((x & 0b111111111111111111111<<10) == 0b101101011000000000111<<10){
        return cnt;
    }
    if(((x & (0x3FFFFF<<10)) == 0b1101011001011111000000<<10) && ((x & 0b11111) == 0b00000)){
        return ret;
    }
    if((x & 0b111111<<26) == 0b100101<<26){
        return bl;
    }

    if((x & 0b111111111<<22) == (0b010100011<<22)){
        return ldp;
    }
    if((x & 0b111111111<<22) == (0b010100101<<22)){
        return ldp;
    }
    if((x & 0b111111111<<22) == (0b010100111<<22)){
        return ldp;
    }
    if((x & (0b1111111 << 24)) == (0b1101011 << 24) && ((x & (0b1 << 21)) == 0)){
        return subs;
    }

    return err;
}

// or is not as simple as just implementing |...
// I used AI to learn more about the functions listed below
uint64_t RotateRight(uint64_t value, int shift, int width) {
    shift %= width;
    // fprintf(stderr, "width = %d, shift = %d, %lx\n", width, shift, ((value >> shift) | (value << (width - shift))));
    return ((value >> shift) | (value << (width - shift)));
}

uint64_t replicate(uint64_t value, int n, int N) {
    uint64_t result = 0;
    for(int i = 0; i < 32*(1+N); i+=n) {
        result |= value << i;
    }
    return result;
}

uint64_t DecodeBitMasks(uint32_t N, uint32_t imms, uint32_t immr, int M){
    uint32_t len = -1;
    uint32_t concat = ((N & 1) << 6) | ((~imms) & 0x3F);
    for (int i = 6; i >= 0; --i) {
        if (concat & (1u << i)) {
            len = i;
            break;
        }
    }


    uint64_t esize = 1ULL <<(len);

    uint64_t S = imms & (esize - 1);
    uint64_t R = immr & (esize - 1);

    uint64_t baseMask = (1ULL <<(S + 1)) - 1;
    // fprintf(stderr, "baseMask = %lx, %ld, %ld\n", baseMask, R, esize);
    
    baseMask = RotateRight(baseMask, R, esize);

    uint64_t fullMask = replicate(baseMask, esize, M);
    return fullMask;
}


uint64_t signExtend(uint64_t x, int n) {
    // see https://graphics.stanford.edu/~seander/bithacks.html#FixedSignExtend
    uint64_t m = 1U << (n - 1);  
    return (x ^ m) - m; 
}

bool processInstruction(int x) {
    
    INSTRUCTION_TYPE type = getType(x);
    // fprintf(stderr, "instruction = %x, type = %d, addr = %lx\n", x, type, pc);
    // fprintf(stderr, "instruction = %x: type = %d\n", x, type);
    switch(type) {
        case cbnz:{
            int rt= (x & 0x1F);
            uint64_t imm19 = (x >> 5) & 0x7FFFF;
            uint64_t cmp = rt==31 ? 0 : registers[rt];
            int M = (x >> 31) & 0x1;
            if(M == 0) {
                cmp = cmp & 0xFFFFFFFF;
            }
            if(cmp != 0) {
                pc += signExtend(imm19, 19) << 2;
                pc -= 4; // to counteract the pc+=4 at the end of the loop
            }
            // fprintf(stderr, "cbnz: cmp = %lx, rt = %d, imm19 = %lx, pc = %lx <- %lx\n", cmp, rt, imm19, pc, opc);
            break;
        }
        case adrp:{
            int rd = (x & 0x1F);
            if(rd == 31) break;
            uint64_t immlo = (x >> 29) & 0x3;
            uint64_t immhi = (x >> 5) & 0x7FFFF;

            uint64_t imm21 = (immhi << 2) | immlo;
            uint64_t offset = signExtend(imm21, 21) << 12;

            registers[rd] = (pc & ~0xFFF) + offset;
            // fprintf(stderr, "adrp: rd = %x, immlo = %lx, immhi = %lx, imm21 = %lx, rd = %lx\n", rd, immlo, immhi, offset, registers[rd]);
            
            break;
        }
        case addi:{
            int rd = (x & 0x1F);
            int rn = (x >> 5) & 0x1F;
            uint64_t imm12 = (x >> 10) & 0xFFF;
            int shift = (((x >> 22) & 0x1) == 0) ? 0 : 12;
            int M = (x >> 31) & 0x1;
            registers[rd] = registers[rn] + (imm12 << shift);
            if(M == 0) {
                registers[rd] = registers[rd] & 0xFFFFFFFF;
            }
            
            break;
        }
        case ldr:{
            // fprintf(stderr, "ldr: %x\n", (x>>24 & 0x1));
            int rd = (x & 0x1F);
            if(rd == 31) break;
            int rn = (x >> 5) & 0x1F;
            int scale = (x >> 30) & 0x3;
            if(x>>24 & 0x1){
                uint64_t imm12 = (x >> 10) & 0xFFF;
                uint64_t address = registers[rn] + (imm12<<scale);
                registers[rd] = mem_read64(address);
                if(scale == 2) {
                    registers[rd] = registers[rd] & 0xFFFFFFFF;
                }
                // fprintf(stderr, "ldr: %x, rd = %x, rn = %x, imm12 = %lx, address = %lx, rd = %lx\n", x, rd, rn, imm12, address, registers[rd]);
                
                break;
            }
            uint64_t imm9 = (x >> 12) & 0x1FF;
            imm9 = signExtend(imm9, 9);
            // fprintf(stderr, "%lx\n", imm9);
            if(x>>11 & 0x1){
               // fprintf(stderr, "ldr: %lx\n", registers[rn]);
                registers[rn] = registers[rn] + imm9;
               // fprintf(stderr, "ldr: %lx\n", registers[rn]);
                registers[rd] = mem_read64(registers[rn]);
                if(scale == 2) {
                    registers[rd] = registers[rd] & 0xFFFFFFFF;
                }
                // fprintf(stderr, "ldr1: %x, rd = %x, rn = %x, imm9 = %lx, rd = %lx\n", x, rd, rn, imm9, registers[rd]);
                
                break;
            }
            registers[rd] = mem_read64(registers[rn]);
            registers[rn] = registers[rn] + imm9;
            if(scale == 2) {
                registers[rd] = registers[rd] & 0xFFFFFFFF;
            }
            // fprintf(stderr, "ldr2: %x, rd = %x, rn = %x, imm9 = %lx, rd = %lx\n", x, rd, rn, imm9, registers[rd]);
            break;

        }
        case ldrb:{
            // fprintf(stderr, "ldr: %x\n", (x>>10 & 0x1));
            int rd = (x & 0x1F);
            if(rd == 31) break;
            int rn = (x >> 5) & 0x1F;
            if(x>>24 & 0x1){
                uint64_t imm12 = (x >> 10) & 0xFFF;
                // fprintf(stderr, "ldr: %lx, pc = %lx\n", imm12, pc);
                registers[rd] = mem_read8(registers[rn] + imm12);
                break;
            }
            uint64_t imm9 = (x >> 12) & 0x1FF;
            imm9 = signExtend(imm9, 9);
            if(x>>11 & 0x1){
                registers[rn] = registers[rn] + imm9;
                registers[rd] = mem_read8(registers[rn]);
                break;
            }
            registers[rd] = mem_read8(registers[rn]);
            registers[rn] = registers[rn] + imm9;
            break;
        }
        case movz:{
            int rd = (x & 0x1F);
            uint64_t imm16 = (x >> 5) & 0xFFFF;
            uint64_t shift = (x >> 21) & 0x3;
            int M = (x >> 31) & 0x1;
            if((shift & 0x2) && (M==0)) return false;
            if(rd == 31) break;
            registers[rd] = imm16 << (shift*16);
            if(M == 0) {
                registers[rd] = registers[rd] & 0xFFFFFFFF;
            }
            // fprintf(stderr, "movz: rd = %x, imm16 = %lx, shift = %lx, rd = %lx\n", rd, imm16, shift, registers[rd]);
            
            break;
        }
        case orr:{
            int rd = (x & 0x1F);
            int rn = (x >> 5) & 0x1F;
            uint64_t imm12 = (x >> 10) & 0xFFF;
            int N = (x >> 22) & 0x1;
            int M = (x >> 31) & 0x1;

            uint64_t immr = (imm12 >> 6) & 0x3F;
            uint64_t imms = imm12 & 0x3F;


            uint64_t imm = DecodeBitMasks(N, imms, immr, M);
            // fprintf(stderr, "orr: m = %x, rd = %x, rn = %lx, imms = %lx, immr = %lx, imm = %lx\n", M, rd, registers[rn], imms, immr, imm);
            uint64_t val = rn == 31 ? 0 : registers[rn];
            if(rn == 31) val = 0;
            registers[rd] = val | imm;
            if(M == 0) {
                registers[rd] = registers[rd] & 0xFFFFFFFF;
            }   
            // fprintf(stderr, "orr: rd = %x, rn = %x, imm12 = %x, rd = %lx\n", rd, rn, imm12, registers[rd]);
            break;
        }
        case strb:{
            // fprintf(stderr, "strb: %x\n", x);
            int rt = (x & 0x1F);
            int rn = (x >> 5) & 0x1F;
            uint8_t data = (rt==31 ? 0 : registers[rt]) & 0xFF;
            uint64_t address;
            if(x>>24 & 0x1){
                int imm12 = (x >> 10) & 0xFFF;
                address = registers[rn] + imm12;
            }
            else if(x>>11 & 0x1){
                int imm9 = (x >> 12) & 0x1FF;
                imm9 = signExtend(imm9, 9);
                registers[rn] = registers[rn] + imm9;
                address = registers[rn];
            }
            else {
                int imm9 = (x >> 12) & 0x1FF;
                imm9 = signExtend(imm9, 9);
                address = registers[rn];
                registers[rn] = registers[rn] + imm9;
            }
            // fprintf(stderr, "strb: pc = %lx, %x, rt = %x, rn = %x, data = %c, rtval = %lx, address = %lx\n", pc, x, rt, rn, data, registers[rt], address);
            mem_write8(address, data);
            break;
        }
        case str:{
            int rt = (x & 0x1F);
            int rn = (x >> 5) & 0x1F;
            int scale = (x >> 30) & 0x3;
            uint64_t address;
            uint64_t value = rt == 31 ? 0 : registers[rt];
            if((x >> 24) & 0b1){
                uint64_t imm12 = (x >> 10) & 0xFFF;
                address = registers[rn] + (imm12<<scale);
            }
            else{
                uint64_t imm9 = (x >> 12) & 0x1FF;
                imm9 = signExtend(imm9, 9);
                if((x >> 11) & 0b1){
                    registers[rn] = registers[rn] + imm9;
                    address = registers[rn];
                }
                else{
                    address = registers[rn];
                    registers[rn] = registers[rn] + imm9;
                }
            }
            if((x & 0b1<<30) == 0){
                mem_write32(address, value);
            }
            else{
                mem_write64(address, value);
            }
            break;
        }
        case ldp:{
            int identifier = (x >> 23) & 0x3;
            int rt = (x & 0x1F);
            int rn = (x >> 5) & 0x1F;
            int rt2 = (x >> 10) & 0x1F;
            int M = (x >> 31) & 0x1;
            int scale = 4 * (1+M);
            uint64_t address;
            uint64_t imm7 = (x >> 15) & 0x7F;
            imm7 = signExtend(imm7, 7);
            if(identifier == 1){
                // post index
                address = registers[rn];
                // fprintf(stderr, "ldp, imm7 = %lx, scale = %d, address = %lx\n", imm7, scale, address);
                registers[rn] = registers[rn] + imm7*scale;
            }
            else if(identifier == 2){
                // offset
                address = registers[rn] + imm7*scale;
            }
            else if(identifier == 3){
                // pre index
                registers[rn] = registers[rn] + imm7*scale;
                address = registers[rn];
            }
            if(M){
                if(rt!=31) registers[rt] = mem_read64(address);
                if(rt2!=31) registers[rt2] = mem_read64(address+8);
            }
            else{
                if(rt!=31) registers[rt] = mem_read32(address);
                if(rt2!=31) registers[rt2] = mem_read32(address+4);
            }
            break;
        }
        case bl:{
            uint64_t imm26 = (x & 0x3FFFFFF);
            imm26 = signExtend(imm26, 26);
            registers[30] = pc+4;
            pc = pc + imm26*4 - 4;
            break;
        }
        case ret:{
            int rn = (x>>5) & 0x1F;
            pc = (rn == 31 ? 0 : registers[rn])-4;
            break;
        }
        case cnt:{
            int rd = (x & 0x1F);
            int rn = (x >> 5) & 0x1F;
            if(rd == 31) break;
            if(rn == 31){
                registers[rd] = 0;
                break;
            }
            if((x & 0b1<<31) == 0){
                registers[rd] = __builtin_popcount(registers[rn]);
            }
            else{
                registers[rd] = __builtin_popcountll(registers[rn]);
            }
            // fprintf(stderr, "cnt: rd = %x, rn = %x, rd = %lx\n", rd, rn, registers[rd]);
            break;
        }
        case sdiv:{
            int rd = (x & 0x1F);
            int rn = (x >> 5) & 0x1F;
            int rm = (x >> 16) & 0x1F;
            if(rd == 31) break;
            //fprintf(stderr, "sdiv: %lx, %lx, %lx\n", registers[rn], registers[rm], registers[rd]);
            uint64_t rnVal = rn == 31 ? 0 : registers[rn];
            uint64_t rmVal = rm == 31 ? 0 : registers[rm];
            if((x & 0b1<<31) == 0){
                if((rmVal&0xFFFFFFFF) == 0){
                    registers[rd] = 0;
                    break;
                }
                //fprintf(stderr, "sdivIN: %lx, %lx, %lx\n", registers[rn], registers[rm], registers[rd]);
                registers[rd] = (static_cast<int>(rnVal&0xFFFFFFFF) / static_cast<int>(rmVal&0xFFFFFFFF))&0xFFFFFFFF;
            }
            else{
                if(rmVal == 0){
                    registers[rd] = 0;
                    break;
                }
                //fprintf(stderr, "sdivl: %lx, %lx, %lx\n", registers[rn], registers[rm], registers[rd]);
                registers[rd] = static_cast<long long>(rnVal) / static_cast<long long>(rmVal);
            }
            break;
        }
        case subs:{
            int rd = (x & 0x1F);
            int rn = (x >> 5) & 0x1F;
            uint64_t imm = (x >> 10) & 0x3F;
            int rm = (x >> 16) & 0x1F;
            imm = signExtend(imm, 6);
            int sh = (x >> 22) & 0x3;
            int sf = (x >> 31) & 0x1;
            uint64_t rmVal = rm == 31 ? 0 : registers[rm];
            //fprintf(stderr, "imm : %ld, val: %ld, sh : %d, rn: %d, rd: %d\n", imm, registers[rm], sh, rn, rd);
            uint64_t mask = 0xFFFFFFFFFFFFFFFF;
            if(sf == 0){
                mask = 0xFFFFFFFF;
            }
            if(sh == 0){
                imm = (rmVal&mask) << imm;
            }
            else if(sh == 1){
                // fprintf(stderr, "imm : %lx, rm : %ld, res : %ld\n", imm, registers[rm], registers[rm]>>2);
                imm = ((rmVal)&mask) >> imm;
            }
            else if(sh == 2){
                if(sf == 0){
                    imm = static_cast<int32_t>((rmVal)&mask) >> imm;
                }
                else{
                    imm = static_cast<int64_t>((rmVal)&mask) >> imm;
                }
            }
            uint64_t value = rn == 31 ? 0 : registers[rn];
            // fprintf(stderr, "subs: rd = %lx, value = %ld, imm = %ld, rm = %x, rd = %lx\n", registers[rd], value, imm, rm, registers[rd]);
            if(sf == 0){
                value &= 0xFFFFFFFF;
            }
            if(rd != 31) registers[rd] = value - imm;
            uint64_t result = rd == 31 ? 0 : registers[rd];
            //fprintf(stderr, "result: %ld\n", registers[rd]);
            if(sf == 0){
                result &= 0xFFFFFFFF;
                if(rd != 31) registers[rd] = result;
                N = (result & (0x1<<31)) == 1ULL<<31;
                // a^b will have the msb being 1 if a and b have different signs
                // so, we check that value and imm and value and res have different signs
                V = (((value ^ imm) & (value ^ result)) & 1ULL<<31) == 1ULL<<31;
            }
            else{
                N = (result & (0x1ULL<<63)) == 1ULL<<63;
                V = (((value ^ imm) & (value ^ result)) & (1ULL<<63)) == 1ULL<<63;
            }
            Z = (result == 0);
            C = (value >= imm);
            break;
        }
        case csel:{
            int rd = (x & 0x1F);
            if(rd == 31) break;
            int rn = (x >> 5) & 0x1F; 
            int rm = (x >> 16) & 0x1F;
            int cond = (x >> 12) & 0xF;
            bool condition = false;
            switch (cond){
                case 0:
                    condition = Z;
                    break;
                case 1:
                    condition = !Z;
                    break;
                case 2:
                    condition = C;
                    break;
                case 3:
                    condition = !C;
                    break;
                case 4:
                    condition = N;
                    break;
                case 5:
                    condition = !N;
                    break;
                case 6:
                    condition = V;
                    break;
                case 7:
                    condition = !V;
                    break;
                case 8:
                    condition = C && !Z;
                    break;
                case 9:
                    condition = !C || Z;
                    break;
                case 10:
                    condition = N == V;
                    break;
                case 11:
                    condition = N != V;
                    break;
                case 12:
                    condition = !Z && (N == V);
                    break;
                case 13:
                    condition = Z || (N != V);
                    break;
                case 14:
                    condition = true;
                    break;
                case 15:
                    condition = false;
                    break;
                default:
                    break;
            }
            registers[rd] = condition ? 
                (rn == 31 ? 0 : registers[rn]) : 
                (rm == 31 ? 0 : registers[rm]);
            if((x & 0x1<<31) == 0){
                registers[rd] = registers[rd] & 0xFFFFFFFF;
            }
            break;
        }
        case err:
            return false;
            break;
    }
    return true;
}

int main(int argc, char** argv) {
    if (argc != 2) {
        printf("usage: %s <ARM executable file name>\n",argv[0]);
        exit(-1);
    }
    for(int i = 0; i<32; i++){
        registers[i] = 0;
    }
    loadElf(argv[1]);
    pc = entry;
    int x;
    while(true) {
        x = mem_read32(pc);
        bool passed = processInstruction(x);
        if(!passed){
            printf("unknown instruction %08x at %lx\n", x, pc);
            for(int i = 0; i<10; i++){
                printf("X0%d : %016lX\n", i, registers[i]);
            }
            for(int i = 10; i<31; i++){
                printf("X%d : %016lX\n", i, registers[i]);
            }
            printf("XSP : %016lX\n", registers[31]);
            free(memory);
            return 0;
        }
        
        pc += 4;
    }
    return 0;
}
