/*
 * This file is part of the SymbosVM project, which is distributed
 * under the terms of the GNU General Public License; either
 * version 2 of the License, or (at your option) any later version.
 */

// written by insane/Rabenauge^.tSCc.

#include <math.h>
#include <stdint.h>
#include "system.h"
#include "config.h"

uint32_t beeps[256];
int audioframesize;
uint32_t beepfreq,beepacc;

int16_t beep_synth() {
  beepacc+=beepfreq;
  return((beepacc&0x80000000)>>18);
};

void audio_update() {
};

void audio_reset() {
};

void audio_beep(uint8_t freq) {
  beepfreq=beeps[freq];
  if (beepfreq==0) beepacc=0;
};

void initbeeps() {
  double microtone=pow(2,1.0/48);
  for(int i=0;i<256;i++) beeps[i]=0;
  for(int i=4;i<64*4;i++) {
    beeps[i]=round(440*pow(microtone,i-37*4)/var_audio_samplerate*0x100000000);
  }
};

int init_audio() {
  initbeeps();
  audio_beep(0);
  audioframesize=var_audio_samplerate/sys_timer_freq;
  return 1;
};

int done_audio() {
  return 1;
};

/* vim: set et ts=2 sw=2 :*/
