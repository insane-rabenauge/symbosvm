/*
 * This file is part of the SymbosVM project, which is distributed
 * under the terms of the GNU General Public License; either
 * version 2 of the License, or (at your option) any later version.
 */

// written by insane/Rabenauge^.tSCc.

// SymbosVM Audio updates every 50hz

#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include "system.h"
#include "config.h"
#include "audio_ym.h"

#define YMFREQ_CPC 1000000
#define YMFREQ_ZX  1773400
#define YMFREQ_MSX 1789772
#define YMFREQ_ST  2000000

int ymfreqs[16]={0,1000000,1773400,1789772,2000000};

ayemu_ay_t psg1;
ayemu_ay_t psg2;
ayemu_ay_reg_frame_t psg1regs;
ayemu_ay_reg_frame_t psg2regs;
int audqueueold=0;

void audio_psg1set(int sel,int dat) {
  psg1regs[sel&15]=dat;
};

void audio_psg2set(int sel,int dat) {
  psg2regs[sel&15]=dat;
};

void audio_psg1ctrl(int dat) {
  char* pan=AY_MONO;
  switch (dat>>4) {
    case 0: pan=AY_MONO;break;
    case 1: pan=AY_SABC;break;
    case 2: pan=AY_SACB;break;
  };
  int freq=ymfreqs[dat&15];

  ayemu_init(&psg1,freq,var_audio_samplerate,pan);
};

void audio_psg2ctrl(int dat) {
  char* pan=AY_MONO;
  switch (dat>>4) {
    case 0: pan=AY_MONO;break;
    case 1: pan=AY_SABC;break;
    case 2: pan=AY_SACB;break;
  };
  int freq=ymfreqs[dat&15];

  ayemu_init(&psg2,freq,var_audio_samplerate,pan);
};


uint32_t beeps[256];
int audioframesize;
uint32_t beepfreq,beepacc;

int16_t beep_synth() {
  beepacc+=beepfreq;
  return((beepacc&0x80000000)>>18);
};

uint32_t aud_mix(uint32_t out, uint32_t out2) {
  int32_t out_l = (out & 0xffff) + (out2 & 0xffff);
  int32_t out_r = (out >> 16) + (out2 >> 16);
  if (out_l > 32767)
    out_l = 32767;
  if (out_r > 32767)
    out_r = 32767;
  return out_l | (out_r << 16);
}

int audio_update() {
  int audon=0,smpcnt=0,smpadd=0;

// calc samples and push to system
  ayemu_set_regs(&psg1,psg1regs);

// check if any sound chip is enabled
  if (psg1.ChipFreq) audon=1;
  if (psg2.ChipFreq) audon=1;

  if (audon) {
    smpcnt=audioframesize;

// keep audio queue filled - this is in NO WAY exact - but neither is the 50hz interrupt
    smpadd=(var_audio_buffersize/2)-system_audqueuesize();
    if (smpadd>0) smpcnt+=smpadd;

// generate samples
    for (int smp=0;smp<smpcnt;smp++) {
      uint32_t csmp=0;
      if (psg1.ChipFreq) csmp=aud_mix(csmp,ayemu_mix(&psg1));
      if (psg2.ChipFreq) csmp=aud_mix(csmp,ayemu_mix(&psg2));
      system_audqueueadd(csmp);
    };
  };

  return (smpadd>0);
};

void audio_reset() {
// clear queue
  system_audqueueclear();
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
  ayemu_init(&psg1,0,var_audio_samplerate,AY_MONO);
  ayemu_init(&psg2,0,var_audio_samplerate,AY_MONO);

  return 1;
};

int done_audio() {
  return 1;
};

/* vim: set et ts=2 sw=2 :*/
