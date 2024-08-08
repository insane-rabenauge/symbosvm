/*
 * This file is part of the SymbosVM project, which is distributed
 * under the terms of the GNU General Public License; either
 * version 2 of the License, or (at your option) any later version.
 */

// written by insane/Rabenauge^.tSCc.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "simz80.h"
#include "hardware.h"
#include "system.h"
#include "audio.h"
#include "video.h"
#include "config.h"

volatile uint32_t ticks_framestart=0;
uint8_t *z80_rom;
uint32_t z80_rom_size=0;

int z80_loadrom(char * fnam) {
  FILE *f;
  if ((f=fopen(fnam,"rb"))==NULL) {
    printf("error loading file %s\n",fnam);
    return(0);
  };
  fseek(f,0,SEEK_END);
  z80_rom_size=ftell(f);
  rewind(f);
  if (z80_rom_size>=MEM_SIZE) {
    printf("file too big for symbosvm\n");
    fclose(f);
    return(0);
  };
  z80_rom=malloc(z80_rom_size);
  if (z80_rom==NULL) {
    printf("error allocating ROM memory\n");
    fclose(f);
    return(0);
  };
  fread(z80_rom,z80_rom_size,1,f);
  fclose(f);
  return(1);
};

int init_z80() {
  for (int bank=0;bank<4;bank++) simz80_setbank(bank,bank);

  z80_regs r;
  r.pc=r.sp=r.af=r.bc=r.de=r.hl=r.ix=r.iy=r.ir=r.IFF=0;
  simz80_setregs(&r);

  memcpy(z80_mem,z80_rom,z80_rom_size);

  return 1;
};

void done_z80() {
};

void z80_update() {
  z80_running=1;
  ticks_framestart=system_gettickus();
  simz80_run();
  if(!sys_timer_irq)system_waitfortimer();
  sys_timer_irq=0;
  simz80_irq(sys_timer_vector);
  if (sys_quit_type==D_VMREBOOT) {
    sys_quit_type=D_VMEXIT;
    init_z80();

    video_reset();
    audio_reset();
    system_reset();
  };
};

int z80_execute() {
  while(!sys_quit) {
    z80_update();
    video_update();
    audio_update();
    system_update();
  };
  return sys_quit;
};
