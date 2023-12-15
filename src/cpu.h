#ifndef CURSES6502_CPU_H
#define CURSES6502_CPU_H

#include <stdint.h>

#define FLAG_CARRY (1 << 0)
#define FLAG_ZERO (1 << 1)
#define FLAG_INTERRUPT (1 << 2)
#define FLAG_DECIMAL (1 << 3)
#define FLAG_BREAK (1 << 4)
#define FLAG_UNUSED (1 << 5)
#define FLAG_OVERFLOW (1 << 6)
#define FLAG_NEGATIVE (1 << 7)

#define ADDR_IMP  1
#define ADDR_IMM  2
#define ADDR_ZP   3
#define ADDR_ZPX  4
#define ADDR_ZPY  5
#define ADDR_REL  6
#define ADDR_ABSO 7
#define ADDR_ABSX 8
#define ADDR_ABSY 9
#define ADDR_IND  10
#define ADDR_INDX 11
#define ADDR_INDY 12

#define SETFLAG(flag, value) if (value) { status |= flag; } else { status &= ~(flag); }
#define FLAGSET(flag) ((status & flag) != 0)
#define FLAGCLEAR(flag) !FLAGSET(flag)

extern uint16_t pc;
extern uint8_t sp;
extern uint8_t status;
extern uint8_t a;
extern uint8_t x;
extern uint8_t y;

extern uint8_t instruction;

extern uint8_t cpu_memory[0x10000];

uint8_t read8(uint16_t address);
void write8(uint16_t address, uint8_t value);

void push16(uint16_t value);
void push8(uint8_t value);

uint16_t pull16(void);
uint8_t pull8(void);

void cpu_tick(void);
void cpu_next_instruction(void);

void cpu_reset(void);

void imp(void);
void imm(void);
void zp(void);
void zpx(void);
void zpy(void);
void rel(void);
void abso(void);
void absx(void);
void absy(void);
void ind(void);
void indx(void);
void indy(void);

void adc(void);
void and(void);
void asl(void);
void bcc(void);
void bcs(void);
void beq(void);
void bit(void);
void bmi(void);
void bne(void);
void bpl(void);
void brk(void);
void bvc(void);
void bvs(void);
void clc(void);
void cld(void);
void cli(void);
void clv(void);
void cmp(void);
void cpx(void);
void cpy(void);
void dec(void);
void dex(void);
void dey(void);
void eor(void);
void inc(void);
void inx(void);
void iny(void);
void jmp(void);
void jsr(void);
void lda(void);
void ldx(void);
void ldy(void);
void lsr(void);
void nop(void);
void ora(void);
void pha(void);
void php(void);
void pla(void);
void plp(void);
void rol(void);
void ror(void);
void rti(void);
void rts(void);
void sbc(void);
void sec(void);
void sed(void);
void sei(void);
void sta(void);
void stx(void);
void sty(void);
void tax(void);
void tay(void);
void tsx(void);
void txa(void);
void txs(void);
void tya(void);

#endif
