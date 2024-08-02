/*
 * This file is part of the SymbosVM project, which is distributed
 * under the terms of the GNU General Public License; either
 * version 2 of the License, or (at your option) any later version.
 */

// written by insane/Rabenauge^.tSCc.

#include <stdio.h>
#include "simz80.h"
#include "system.h"
#include "audio.h"
#include "video.h"
#include "config.h"

volatile uint32_t ticks_framestart=0;

int init_z80() {
  for (int bank=0;bank<4;bank++) simz80_setbank(bank,bank);

  z80_regs r;
  r.pc=r.sp=r.af=r.bc=r.de=r.hl=r.ix=r.iy=r.ir=r.IFF=0;
  simz80_setregs(&r);

  return 1;
};

int run_z80() {
  while(!sys_quit) {
    z80_run=1;
    ticks_framestart=system_gettickus();
    simz80_run();
    if(!sys_timer_irq)system_waitfortimer();
    sys_timer_irq=0;
    simz80_irq(sys_timer_vector);
    video_update();
    audio_update();
    system_update();
  };
  return 1;
};
