#ifndef _VMZ80_H
#define _VMZ80_H
extern volatile uint32_t ticks_framestart;
int init_z80();
int z80_execute();
int done_z80();
int z80_loadrom(char * fnam);
#endif
