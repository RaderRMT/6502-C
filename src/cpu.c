#include "cpu.h"

// 6502 registers
uint16_t pc;
uint8_t sp;
uint8_t status;
uint8_t a;
uint8_t x;
uint8_t y;

uint8_t cpu_memory[0x10000] = {0 };

uint8_t cycles = 0;
uint8_t fetched = 0;
uint16_t relative_address = 0;
uint16_t absolute_address = 0;

uint8_t addr_mode = 0;

uint8_t instruction = 0;

void (*addr_modes[256])(void);
void (*opcodes[256])(void);
uint8_t instruction_cycles[256];

uint8_t read8(uint16_t address) {
    return cpu_memory[address];
}

uint16_t read16(uint16_t address) {
    uint8_t lo = read8(address);
    uint8_t hi = read8(address + 1);

    return (uint16_t) hi << 8 | lo;
}

void write8(uint16_t address, uint8_t value) {
    cpu_memory[address] = value;
}

void push16(uint16_t value) {
    cpu_memory[0x0100 + sp--] = value >> 8 & 0xff;
    cpu_memory[0x0100 + sp--] = value & 0xff;
}

void push8(uint8_t value) {
    cpu_memory[0x0100 + sp--] = value;
}

uint16_t pull16(void) {
    uint16_t pulled = cpu_memory[0x0100 + ++sp];
    pulled |= (uint16_t) cpu_memory[0x0100 + ++sp] << 8;

    return pulled;
}

uint8_t pull8(void) {
    return cpu_memory[0x0100 + ++sp];
}

void cpu_tick(void) {
    if (cycles != 0) {
        cycles--;
        return;
    }

    instruction = read8(pc++);
    (*addr_modes[instruction])();
    (*opcodes[instruction])();
    cycles = instruction_cycles[instruction];
}

void cpu_next_instruction(void) {
    cpu_tick();
    cycles = 0;
}

void cpu_reset(void) {
    // get the reset vector from the rom and
    // set the program counter to the reset vector
    pc = read16(0xFFFC);

    // set the IRQB disable flag and the Unused flag to 1
    // and set the Decimal flag to 0
    //
    // bit:     76543210
    // flag:    NVUBDIZC
    //          ||||||||
    status |= 0b00100100;
    status &= 0b11110111;
    // we ignore bit 0, 1, 6, and 7 since they
    // are not initialised by the reset sequence

    // the reset sequence lasts 7 clock cycles
    cycles = 7;
}

void imp(void) {
    addr_mode = ADDR_IMP;
    fetched = a;
}

void imm(void) {
    addr_mode = ADDR_IMM;
    fetched = read8(pc++);
}

void zp(void) {
    addr_mode = ADDR_ZP;
    absolute_address = read8(pc++);
    fetched = read8(absolute_address);
}

void zpx(void) {
    addr_mode = ADDR_ZPX;
    absolute_address = read8(pc++) + x;
    fetched = read8(absolute_address);
}

void zpy(void) {
    addr_mode = ADDR_ZPY;
    absolute_address = read8(pc++) + y;
    fetched = read8(absolute_address);
}

void rel(void) {
    addr_mode = ADDR_REL;
    int8_t offset = (int8_t) read8(pc++);
    relative_address = pc + offset;
}

void abso(void) {
    addr_mode = ADDR_ABSO;
    absolute_address = read16(pc);
    pc += 2;
}

void absx(void) {
    addr_mode = ADDR_ABSX;
    uint16_t base = read16(pc);
    absolute_address = base + x;

    fetched = read8(absolute_address);
    pc += 2;

    if ((absolute_address & 0xff00) != (base & 0xff00)) {
        cycles++;
    }
}

void absy(void) {
    addr_mode = ADDR_ABSY;
    uint16_t base = read16(pc);
    absolute_address = base + y;

    fetched = read8(absolute_address);
    pc += 2;

    if ((absolute_address & 0xff00) != (base & 0xff00)) {
        cycles++;
    }
}

void ind(void) {
    addr_mode = ADDR_IND;
    absolute_address = read16(read16(pc));
    pc += 2;
}

void indx(void) {
    addr_mode = ADDR_INDX;
    absolute_address = read16(read8(pc++) + x);
    fetched = read8(absolute_address);
}

void indy(void) {
    addr_mode = ADDR_INDY;
    uint16_t base = read16(read8(pc++));
    absolute_address = base + y;

    fetched = read8(absolute_address);

    if ((absolute_address & 0xff00) != (base & 0xff00)) {
        cycles++;
    }
}

void adc(void) {
    uint16_t temp = a + fetched;

    SETFLAG(FLAG_CARRY, temp > 255)
    SETFLAG(FLAG_ZERO, temp == 0)
    SETFLAG(FLAG_OVERFLOW, (~(a ^ fetched) & (a ^ temp)) & 0x80)
    SETFLAG(FLAG_NEGATIVE, temp & 0x80)

    a = temp;
}

void and(void) {
    a &= fetched;

    SETFLAG(FLAG_ZERO, a == 0)
    SETFLAG(FLAG_NEGATIVE, a & 0x80)
}

void asl(void) {
    uint16_t temp = fetched << 1;

    SETFLAG(FLAG_CARRY, temp & 0xff00)
    SETFLAG(FLAG_ZERO, (temp & 0x00ff) == 0)
    SETFLAG(FLAG_NEGATIVE, temp & 0x80)

    if (addr_mode == ADDR_IMP) {
        a = temp;
    } else {
        write8(absolute_address, temp);
    }
}

void bcc(void) {
    if (FLAGSET(FLAG_CARRY)) {
        return;
    }

    absolute_address = relative_address;
    if ((absolute_address & 0xff00) != (pc & 0xff00)) {
        cycles++;
    }

    cycles++;
    pc = absolute_address;
}

void bcs(void) {
    if (FLAGCLEAR(FLAG_CARRY)) {
        return;
    }

    absolute_address = relative_address;
    if ((absolute_address & 0xff00) != (pc & 0xff00)) {
        cycles++;
    }

    cycles++;
    pc = absolute_address;
}

void beq(void) {
    if (FLAGCLEAR(FLAG_ZERO)) {
        return;
    }

    absolute_address = relative_address;
    if ((absolute_address & 0xff00) != (pc & 0xff00)) {
        cycles++;
    }

    cycles++;
    pc = absolute_address;
}

void bit(void) {
    uint8_t temp = a & fetched;

    SETFLAG(FLAG_ZERO, temp == 0)
    SETFLAG(FLAG_NEGATIVE, temp & 0x80)
    SETFLAG(FLAG_OVERFLOW, temp & 0x40)
}

void bmi(void) {
    if (FLAGCLEAR(FLAG_NEGATIVE)) {
        return;
    }

    absolute_address = relative_address;
    if ((absolute_address & 0xff00) != (pc & 0xff00)) {
        cycles++;
    }

    cycles++;
    pc = absolute_address;
}

void bne(void) {
    if (FLAGSET(FLAG_ZERO)) {
        return;
    }

    absolute_address = relative_address;
    if ((absolute_address & 0xff00) != (pc & 0xff00)) {
        cycles++;
    }

    cycles++;
    pc = absolute_address;
}

void bpl(void) {
    if (FLAGSET(FLAG_NEGATIVE)) {
        return;
    }

    absolute_address = relative_address;
    if ((absolute_address & 0xff00) != (pc & 0xff00)) {
        cycles++;
    }

    cycles++;
    pc = absolute_address;
}

void brk(void) {
    pc++;

    SETFLAG(FLAG_INTERRUPT, 1)
    push16(pc);

    push8(status | FLAG_BREAK);

    pc = read16(0xFFFE);
}

void bvc(void) {
    if (FLAGSET(FLAG_OVERFLOW)) {
        return;
    }

    absolute_address = relative_address;
    if ((absolute_address & 0xff00) != (pc & 0xff00)) {
        cycles++;
    }

    cycles++;
    pc = absolute_address;
}

void bvs(void) {
    if (FLAGCLEAR(FLAG_OVERFLOW)) {
        return;
    }

    absolute_address = relative_address;
    if ((absolute_address & 0xff00) != (pc & 0xff00)) {
        cycles++;
    }

    cycles++;
    pc = absolute_address;
}

void clc(void) {
    SETFLAG(FLAG_CARRY, 0)
}

void cld(void) {
    SETFLAG(FLAG_DECIMAL, 0)
}

void cli(void) {
    SETFLAG(FLAG_INTERRUPT, 0)
}

void clv(void) {
    SETFLAG(FLAG_OVERFLOW, 0)
}

void cmp(void) {
    uint8_t temp = a - fetched;

    SETFLAG(FLAG_CARRY, a >= fetched)
    SETFLAG(FLAG_ZERO, (temp & 0xff) == 0)
    SETFLAG(FLAG_NEGATIVE, temp & 0x80)
}

void cpx(void) {
    uint8_t temp = x - fetched;

    SETFLAG(FLAG_CARRY, x >= fetched)
    SETFLAG(FLAG_ZERO, (temp & 0xff) == 0)
    SETFLAG(FLAG_NEGATIVE, temp & 0x80)
}

void cpy(void) {
    uint8_t temp = y - fetched;

    SETFLAG(FLAG_CARRY, y >= fetched)
    SETFLAG(FLAG_ZERO, (temp & 0xff) == 0)
    SETFLAG(FLAG_NEGATIVE, temp & 0x80)
}

void dec(void) {
    uint8_t temp = fetched - 1;

    SETFLAG(FLAG_ZERO, (temp & 0xff) == 0)
    SETFLAG(FLAG_NEGATIVE, temp & 0x80)

    write8(absolute_address, temp);
}

void dex(void) {
    x--;

    SETFLAG(FLAG_ZERO, (x & 0xff) == 0)
    SETFLAG(FLAG_NEGATIVE, x & 0x80)
}

void dey(void) {
    y--;

    SETFLAG(FLAG_ZERO, (y & 0xff) == 0)
    SETFLAG(FLAG_NEGATIVE, y & 0x80)
}

void eor(void) {
    a ^= fetched;

    SETFLAG(FLAG_ZERO, a == 0)
    SETFLAG(FLAG_NEGATIVE, a & 0x80)
}

void inc(void) {
    uint8_t temp = fetched + 1;

    SETFLAG(FLAG_ZERO, temp == 0)
    SETFLAG(FLAG_NEGATIVE, temp & 0x80)

    write8(absolute_address, temp);
}

void inx(void) {
    x++;

    SETFLAG(FLAG_ZERO, (x & 0xff) == 0)
    SETFLAG(FLAG_NEGATIVE, x & 0x80)
}

void iny(void) {
    y++;

    SETFLAG(FLAG_ZERO, (x & 0xff) == 0)
    SETFLAG(FLAG_NEGATIVE, x & 0x80)
}

void jmp(void) {
    pc = absolute_address;
}

void jsr(void) {
    push16(--pc);
    pc = absolute_address;
}

void lda(void) {
    a = fetched;

    SETFLAG(FLAG_ZERO, a == 0)
    SETFLAG(FLAG_NEGATIVE, a & 0x80)
}

void ldx(void) {
    x = fetched;

    SETFLAG(FLAG_ZERO, x == 0)
    SETFLAG(FLAG_NEGATIVE, x & 0x80)
}

void ldy(void) {
    y = fetched;

    SETFLAG(FLAG_ZERO, y == 0)
    SETFLAG(FLAG_NEGATIVE, y & 0x80)
}

void lsr(void) {
    SETFLAG(FLAG_CARRY, fetched & 1)

    uint8_t temp = fetched >> 1;
    SETFLAG(FLAG_ZERO, temp == 0)
    SETFLAG(FLAG_NEGATIVE, temp & 0x80)

    if (addr_mode == ADDR_IMP) {
        a = temp;
    } else {
        write8(absolute_address, temp);
    }
}

void nop(void) {
}

void ora(void) {
    a |= fetched;

    SETFLAG(FLAG_ZERO, a == 0)
    SETFLAG(FLAG_NEGATIVE, a & 0x80)
}

void pha(void) {
    push8(a);
}

void php(void) {
    push8(status | FLAG_BREAK);
}

void pla(void) {
    a = pull8();

    SETFLAG(FLAG_ZERO, a == 0)
    SETFLAG(FLAG_NEGATIVE, a & 0x80)
}

void plp(void) {
    status = pull8();
}

void rol(void) {
    uint16_t temp = fetched << 1 | FLAGSET(FLAG_CARRY);

    SETFLAG(FLAG_CARRY, temp & 0xff00)
    SETFLAG(FLAG_ZERO, (temp & 0xff) == 0)
    SETFLAG(FLAG_NEGATIVE, temp & 0x80)

    if (addr_mode == ADDR_IMP) {
        a = temp;
    } else {
        write8(absolute_address, temp);
    }
}

void ror(void) {
    uint16_t temp = (FLAGSET(FLAG_CARRY) << 7) | (fetched >> 1);

    SETFLAG(FLAG_CARRY, temp & 0xff00)
    SETFLAG(FLAG_ZERO, (temp & 0xff) == 0)
    SETFLAG(FLAG_NEGATIVE, temp & 0x80)

    if (addr_mode == ADDR_IMP) {
        a = temp;
    } else {
        write8(absolute_address, temp);
    }
}

void rti(void) {
    status = pull8();
    status &= ~FLAG_CARRY;

    pc = pull16();
}

void rts(void) {
    pc = pull16();
    pc++;
}

void sbc(void) {
    uint16_t value = fetched ^ 0xff;
    uint16_t temp = a + value + FLAGSET(FLAG_CARRY);

    SETFLAG(FLAG_CARRY, temp & 0xff00)
    SETFLAG(FLAG_ZERO, (temp & 0x00ff) == 0)
    SETFLAG(FLAG_OVERFLOW, (~(a ^ fetched) & (a ^ temp)) & 0x80)
    SETFLAG(FLAG_NEGATIVE, temp & 0x80)

    a = temp;
}

void sec(void) {
    SETFLAG(FLAG_CARRY, 1)
}

void sed(void) {
    SETFLAG(FLAG_DECIMAL, 1)
}

void sei(void) {
    SETFLAG(FLAG_INTERRUPT, 1)
}

void sta(void) {
    write8(absolute_address, a);
}

void stx(void) {
    write8(absolute_address, x);
}

void sty(void) {
    write8(absolute_address, y);
}

void tax(void) {
    x = a;

    SETFLAG(FLAG_ZERO, x == 0)
    SETFLAG(FLAG_NEGATIVE, x & 0x80)
}

void tay(void) {
    y = a;

    SETFLAG(FLAG_ZERO, y == 0)
    SETFLAG(FLAG_NEGATIVE, y & 0x80)
}

void tsx(void) {
    x = sp;

    SETFLAG(FLAG_ZERO, x == 0)
    SETFLAG(FLAG_NEGATIVE, x & 0x80)
}

void txa(void) {
    a = x;

    SETFLAG(FLAG_ZERO, a == 0)
    SETFLAG(FLAG_NEGATIVE, a & 0x80)
}

void txs(void) {
    sp = x;
}

void tya(void) {
    a = y;

    SETFLAG(FLAG_ZERO, a == 0)
    SETFLAG(FLAG_NEGATIVE, a & 0x80)
}

void (*addr_modes[256])(void) = {
        imm,  indx, imp, imp, imp, zp,  zp,  imp, imp, imm,  imp, imp, imp,  abso, abso, imp,
        rel,  indy, imp, imp, imp, zpx, zpx, imp, imp, absy, imp, imp, imp,  absx, absx, imp,
        abso, indx, imp, imp, zp,  zp,  zp,  imp, imp, imm,  imp, imp, abso, abso, abso, imp,
        rel,  indy, imp, imp, imp, zpx, zpx, imp, imp, absy, imp, imp, imp,  absx, absx, imp,
        imp,  indx, imp, imp, imp, zp,  zp,  imp, imp, imm,  imp, imp, abso, abso, abso, imp,
        rel,  indy, imp, imp, imp, zpx, zpx, imp, imp, absy, imp, imp, imp,  absx, absx, imp,
        imp,  indx, imp, imp, imp, zp,  zp,  imp, imp, imm,  imp, imp, ind,  abso, abso, imp,
        rel,  indy, imp, imp, imp, zpx, zpx, imp, imp, absy, imp, imp, imp,  absx, absx, imp,
        imp,  indx, imp, imp, zp,  zp,  zp,  imp, imp, imp,  imp, imp, abso, abso, abso, imp,
        rel,  indy, imp, imp, zpx, zpx, zpy, imp, imp, absy, imp, imp, imp,  absx, imp,  imp,
        imm,  indx, imm, imp, zp,  zp,  zp,  imp, imp, imm,  imp, imp, abso, abso, abso, imp,
        rel,  indy, imp, imp, zpx, zpx, zpy, imp, imp, absy, imp, imp, absx, absx, absy, imp,
        imm,  indx, imp, imp, zp,  zp,  zp,  imp, imp, imm,  imp, imp, abso, abso, abso, imp,
        rel,  indy, imp, imp, imp, zpx, zpx, imp, imp, absy, imp, imp, imp,  absx, absy, imp,
        imm,  indx, imp, imp, zp,  zp,  zp,  imp, imp, imm,  imp, imp, abso, abso, abso, imp,
        rel,  indy, imp, imp, imp, zpx, zpx, imp, imp, absy, imp, imp, imp,  absx, absx, imp,
};

void (*opcodes[256])(void) = {
        brk, ora, nop, nop, nop, ora, asl, nop, php, ora, asl, nop, nop, ora, asl, nop,
        bpl, ora, nop, nop, nop, ora, asl, nop, clc, ora, nop, nop, nop, ora, asl, nop,
        jsr, and, nop, nop, bit, and, rol, nop, plp, and, rol, nop, bit, and, rol, nop,
        bmi, and, nop, nop, nop, and, rol, nop, sec, and, nop, nop, nop, and, rol, nop,
        rti, eor, nop, nop, nop, eor, lsr, nop, pha, eor, lsr, nop, jmp, eor, lsr, nop,
        bvc, eor, nop, nop, nop, eor, lsr, nop, cli, eor, nop, nop, nop, eor, lsr, nop,
        rts, adc, nop, nop, nop, adc, ror, nop, pla, adc, ror, nop, jmp, adc, ror, nop,
        bvs, adc, nop, nop, nop, adc, ror, nop, sei, adc, nop, nop, nop, adc, ror, nop,
        nop, sta, nop, nop, sty, sta, stx, nop, dey, nop, txa, nop, sty, sta, stx, nop,
        bcc, sta, nop, nop, sty, sta, stx, nop, tya, sta, txs, nop, nop, sta, nop, nop,
        ldy, lda, ldx, nop, ldy, lda, ldx, nop, tay, lda, tax, nop, ldy, lda, ldx, nop,
        bcs, lda, nop, nop, ldy, lda, ldx, nop, clv, lda, tsx, nop, ldy, lda, ldx, nop,
        cpy, cmp, nop, nop, cpy, cmp, dec, nop, iny, cmp, dex, nop, cpy, cmp, dec, nop,
        bne, cmp, nop, nop, nop, cmp, dec, nop, cld, cmp, nop, nop, nop, cmp, dec, nop,
        cpx, sbc, nop, nop, cpx, sbc, inc, nop, inx, sbc, nop, nop, cpx, sbc, inc, nop,
        beq, sbc, nop, nop, nop, sbc, inc, nop, sed, sbc, nop, nop, nop, sbc, inc, nop,
};

uint8_t instruction_cycles[256] = {
        7, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 4, 4, 6, 6,
        2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
        6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 4, 4, 6, 6,
        2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
        6, 6, 2, 8, 3, 3, 5, 5, 3, 2, 2, 2, 3, 4, 6, 6,
        2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
        6, 6, 2, 8, 3, 3, 5, 5, 4, 2, 2, 2, 5, 4, 6, 6,
        2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
        2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
        2, 6, 2, 6, 4, 4, 4, 4, 2, 5, 2, 5, 5, 5, 5, 5,
        2, 6, 2, 6, 3, 3, 3, 3, 2, 2, 2, 2, 4, 4, 4, 4,
        2, 5, 2, 5, 4, 4, 4, 4, 2, 4, 2, 4, 4, 4, 4, 4,
        2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
        2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7,
        2, 6, 2, 8, 3, 3, 5, 5, 2, 2, 2, 2, 4, 4, 6, 6,
        2, 5, 2, 8, 4, 4, 6, 6, 2, 4, 2, 7, 4, 4, 7, 7
};
