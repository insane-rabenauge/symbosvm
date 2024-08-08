#ifndef _SIMZ80_H
#define _SIMZ80_H
#include <stdbool.h>
#include <stdint.h>

typedef struct {
  uint32_t af,bc,de,hl; //upper 16bit: shadow, lower 16bit: normal
  uint16_t ir,ix,iy,sp,pc,im,IFF;
} z80_regs;

extern volatile int z80_running;

int simz80_run(void); // 0=HALT, 1=normal stop
int simz80_irq(int addr);

void simz80_setbank(int reg, int dat);
int simz80_getbank(int reg);

void simz80_getregs(z80_regs *getregs);
void simz80_setregs(z80_regs *setregs);

extern int z80_in(unsigned int);
extern void z80_out(unsigned int, unsigned int);

extern uint8_t z80_mem[];
extern uint8_t *z80_bank[];

#define INLINE inline __attribute__((always_inline))

#define BANK_SIZE 0x4000
#define BANK_MAX 0x100
#define MEM_SIZE (BANK_SIZE*BANK_MAX)
#define BANK_MASK (BANK_SIZE-1)
#define BANK_SHIFT 14
#define MEM_MASK (MEM_SIZE-1)
#define MEM_MASKU (MEM_MASK>>16)
#endif
