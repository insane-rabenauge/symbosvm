/*
 * This file is part of the SymbosVM project, which is distributed
 * under the terms of the GNU General Public License; either
 * version 2 of the License, or (at your option) any later version.
 */

// YM2149 emulator
// based upon Joric's Plain C PT3 Player
// which uses portions of AY_Emul, zxssk, ayemu

#include <stdint.h>
#include "audio_ym.h"

static int AY_table[32] = { 0, 0, 190, 286, 375, 470, 560, 664, 866, 1130, 1515, 1803, 2253, 2848, 3351, 3862, 4844, 6058, 7290, 8559, 10474, 12878, 15297, 17787, 21500, 26172, 30866, 35676, 42664, 50986, 58842, 65535 };

char AY_MONO[] = { 100,100, 100,100, 100,100 };
char AY_SABC[] = { 100, 10,  66, 66,  10,100 };
char AY_SACB[] = { 100, 10,  10,100,  66, 66 };

#define ENVVOL envelope(ay->regs.env_style, ay->env_pos)

static int envelope(int e, int x) {
  int loop = e > 7 && (e % 2) == 0;
  int q = (x / 32) & (loop ? 1 : 3);
  int ofs = (q == 0 ? (e & 4) == 0 : (e == 8 || e == 11 || e == 13 || e == 14)) ? 31 : 0;
  return (q == 0 || loop) ? (ofs + (ofs != 0 ? -1 : 1) * (x % 32)) : ofs;
}

void ayemu_init(ayemu_ay_t * ay, int chipfreq, int freq, char* pan) {
  ay->ChipFreq = chipfreq / 8;
  ay->cnt_a = ay->cnt_b = ay->cnt_c = ay->cnt_n = ay->cnt_e = 0;
  ay->bit_a = ay->bit_b = ay->bit_c = ay->bit_n = 0;
  ay->env_pos = ay->EnvNum = 0;
  ay->Cur_Seed = 0xffff;
  for (int n = 0; n < 32; n++)
    ay->table[n] = AY_table[n * (sizeof(AY_table) / sizeof(AY_table[0])) / 32];
  for (int i = 0; i < 6; i++) ay->eq[i] = pan[i];
  ay->sndfreq = freq;
  ay->ChipStep = 0;
  for (int n = 0; n < 32; n++) {
    for (int m = 0; m < 6; m++)
      ay->vols[m][n] = (int)(((double)ay->table[n] * ay->eq[m]) / 600);
  }
}

void ayemu_set_reg(ayemu_ay_t * ay, int sel, int dat) {
  switch(sel&0xf) {
    case 0x00: ay->regs.tone_a=(ay->regs.tone_a&0x0f00)|dat; break;
    case 0x01: ay->regs.tone_a=(ay->regs.tone_a&0x00ff)|((dat&0xf)<<8); break;
    case 0x02: ay->regs.tone_b=(ay->regs.tone_b&0x0f00)|dat; break;
    case 0x03: ay->regs.tone_b=(ay->regs.tone_b&0x00ff)|((dat&0xf)<<8); break;
    case 0x04: ay->regs.tone_c=(ay->regs.tone_c&0x0f00)|dat; break;
    case 0x05: ay->regs.tone_c=(ay->regs.tone_c&0x00ff)|((dat&0xf)<<8); break;
    case 0x06: ay->regs.noise = dat & 0x1f;break;
    case 0x07:
      ay->regs.R7_tone_a  = !(dat & 0x01);
      ay->regs.R7_tone_b  = !(dat & 0x02);
      ay->regs.R7_tone_c  = !(dat & 0x04);
      ay->regs.R7_noise_a = !(dat & 0x08);
      ay->regs.R7_noise_b = !(dat & 0x10);
      ay->regs.R7_noise_c = !(dat & 0x20);
      break;
    case 0x08:
      ay->regs.vol_a = dat&0x0f;
      ay->regs.env_a = dat&0x10;
      break;
    case 0x09:
      ay->regs.vol_b = dat&0x0f;
      ay->regs.env_b = dat&0x10;
      break;
    case 0x0a:
      ay->regs.vol_c = dat&0x0f;
      ay->regs.env_c = dat&0x10;
      break;
    case 0x0b: ay->regs.env_freq=(ay->regs.env_freq&0xff00)|dat; break;
    case 0x0c: ay->regs.env_freq=(ay->regs.env_freq&0x00ff)|((dat&0xff)<<8); break;
    case 0x0d:
      ay->regs.env_style = dat & 0x0f;
      ay->env_pos = ay->cnt_e = 0;
      break;
  };
}

void ayemu_set_regs(ayemu_ay_t * ay, ayemu_ay_reg_frame_t regs) {
  ay->regs.tone_a = regs[0] + ((regs[1] & 0x0f) << 8);
  ay->regs.tone_b = regs[2] + ((regs[3] & 0x0f) << 8);
  ay->regs.tone_c = regs[4] + ((regs[5] & 0x0f) << 8);
  ay->regs.noise = regs[6] & 0x1f;
  ay->regs.R7_tone_a = !(regs[7] & 0x01);
  ay->regs.R7_tone_b = !(regs[7] & 0x02);
  ay->regs.R7_tone_c = !(regs[7] & 0x04);
  ay->regs.R7_noise_a = !(regs[7] & 0x08);
  ay->regs.R7_noise_b = !(regs[7] & 0x10);
  ay->regs.R7_noise_c = !(regs[7] & 0x20);
  ay->regs.vol_a = regs[8] & 0x0f;
  ay->regs.vol_b = regs[9] & 0x0f;
  ay->regs.vol_c = regs[10] & 0x0f;
  ay->regs.env_a = regs[8] & 0x10;
  ay->regs.env_b = regs[9] & 0x10;
  ay->regs.env_c = regs[10] & 0x10;
  ay->regs.env_freq = regs[11] + (regs[12] << 8);
  if (regs[13] != 0xff) {
    ay->regs.env_style = regs[13] & 0x0f;
    ay->env_pos = ay->cnt_e = 0;
    regs[13]=0xff;
  }
}

uint32_t ayemu_mix(ayemu_ay_t * ay) {
  int mix_l, mix_r;
  int tmpvol;

  while (ay->ChipStep < ay->ChipFreq) {
    if (++ay->cnt_a >= ay->regs.tone_a) {
      ay->cnt_a = 0;
      ay->bit_a = !ay->bit_a;
    }
    if (++ay->cnt_b >= ay->regs.tone_b) {
      ay->cnt_b = 0;
      ay->bit_b = !ay->bit_b;
    }
    if (++ay->cnt_c >= ay->regs.tone_c) {
      ay->cnt_c = 0;
      ay->bit_c = !ay->bit_c;
    }
    if (++ay->cnt_n >= (ay->regs.noise * 2)) {
      ay->cnt_n = 0;
      ay->Cur_Seed = (ay->Cur_Seed * 2 + 1)^(((ay->Cur_Seed >> 16)^(ay->Cur_Seed >> 13))&1);
      ay->bit_n = ((ay->Cur_Seed >> 16) & 1);
    }
    if (++ay->cnt_e >= ay->regs.env_freq) {
      ay->cnt_e = 0;
      if (++ay->env_pos > 127)
        ay->env_pos = 64;
    }
    ay->ChipStep += ay->sndfreq;
  }
  ay->ChipStep -= ay->ChipFreq;
  mix_l = mix_r = 0;
  if ((ay->bit_a | !ay->regs.R7_tone_a) & (ay->bit_n | !ay->regs.R7_noise_a)) {
    tmpvol = (ay->regs.env_a) ? ENVVOL : ay->regs.vol_a * 2 + 1;
    mix_l += ay->vols[0][tmpvol];
    mix_r += ay->vols[1][tmpvol];
  }
  if ((ay->bit_b | !ay->regs.R7_tone_b) & (ay->bit_n | !ay->regs.R7_noise_b)) {
    tmpvol = (ay->regs.env_b) ? ENVVOL : ay->regs.vol_b * 2 + 1;
    mix_l += ay->vols[2][tmpvol];
    mix_r += ay->vols[3][tmpvol];
  }
  if ((ay->bit_c | !ay->regs.R7_tone_c) & (ay->bit_n | !ay->regs.R7_noise_c)) {
    tmpvol = (ay->regs.env_c) ? ENVVOL : ay->regs.vol_c * 2 + 1;
    mix_l += ay->vols[4][tmpvol];
    mix_r += ay->vols[5][tmpvol];
  }
  return mix_l | (mix_r << 16);
}

