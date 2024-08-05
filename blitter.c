/*
 * This file is part of the SymbosVM project, which is distributed
 * under the terms of the GNU General Public License; either
 * version 2 of the License, or (at your option) any later version.
 */

// written by insane/Rabenauge^.tSCc.

#include <stdio.h>
#include <stdint.h>
#include "hardware.h"
#include "simz80.h"
#include "video.h"
#include "config.h"

uint32_t blitsrca;
uint16_t blitsrcl;
uint16_t blitsrcx;
uint16_t blitsrcy;
uint32_t blitdsta;
uint16_t blitdstl;
uint16_t blitdstx;
uint16_t blitdsty;
uint16_t blitsizx;
uint16_t blitsizy;
uint8_t blitfill1;
uint8_t blitfill2;
int blitposx;
int blitposy;
int blitmode;
int pixmode;
int blitbackx;
int blitbacky;
char pixff;

int blitreadsrc() {
  switch (pixmode&3) {
    case 0: //1bpp
      return (z80_mem[blitsrca+((blitposy+blitsrcy)*blitsrcl)+((blitposx+blitsrcx)>>3)]>>((((blitposx+blitsrcx)&7)^7)))&0x1;
    case 1: //2bpp
      return (z80_mem[blitsrca+((blitposy+blitsrcy)*blitsrcl)+((blitposx+blitsrcx)>>2)]>>((((blitposx+blitsrcx)&3)^3)<<1))&0x3;
    case 2: //4bpp
      return (z80_mem[blitsrca+((blitposy+blitsrcy)*blitsrcl)+((blitposx+blitsrcx)>>1)]>>((((blitposx+blitsrcx)&1)^1)<<2))&0xf;
    case 3: //8bpp
      return z80_mem[blitsrca+((blitposy+blitsrcy)*blitsrcl)+blitposx+blitsrcx];
  };
  return 0;
};

int blitreaddst() {
  if (pixmode&4) { //8bpp
    return z80_mem[blitdsta+((blitposy+blitdsty)*blitdstl)+blitposx+blitdstx];
  } else {
    return (z80_mem[blitdsta+((blitposy+blitdsty)*blitdstl)+((blitposx+blitdstx)>>1)]>>((((blitposx+blitdstx)&1)^1)<<2))&0xf;
  };
  return 0;
};

void blitwritedst(int pix) {
  if (pixmode&4) { //8bpp
    z80_mem[blitdsta+((blitposy+blitdsty)*blitdstl)+blitposx+blitdstx]=pix;
  } else {
    int mempos=blitdsta+((blitposy+blitdsty)*blitdstl)+((blitposx+blitdstx)>>1);
    if ((blitposx+blitdstx)&1) { //set low nibble
      z80_mem[mempos]=(z80_mem[mempos]&0xf0)|(pix&0xf);
    } else { //set high nibble
      z80_mem[mempos]=(z80_mem[mempos]&0x0f)|((pix&0xf)<<4);
    };
  };
};

void blitpix() {
  int pix;
  switch(blitmode) {
    case D_BMCOPY:
      blitwritedst(blitreadsrc());
      break;
    case D_BMAND:
      blitwritedst(blitreaddst()&blitreadsrc());
      break;
    case D_BMOR:
      blitwritedst(blitreaddst()|blitreadsrc());
      break;
    case D_BMXOR:
      blitwritedst(blitreaddst()^blitreadsrc());
      break;
    case D_BMFILL:
      blitwritedst(blitfill1);
      break;
    case D_BMSKIP:
      pix=blitreadsrc(); if (pix!=blitfill1) blitwritedst(pix);
      break;
    case D_BMTEXT:
      pix=blitreadsrc(); blitwritedst(pix?blitfill1:blitfill2);
      break;
    case D_BMFXOR:
      blitwritedst(blitreaddst()^(pixff?blitfill2:blitfill1));
      pixff^=1;
      break;
  };
};

void blitline() {
  if (blitbackx) {
    for(blitposx=blitsizx-1;blitposx>=0;blitposx--) {
      blitpix();
    }
  } else {
    for(blitposx=0;blitposx<blitsizx;blitposx++) {
      blitpix();
    };
  };
};

void startblit(uint8_t blitctrl) {
  if(var_debug_blit)
  printf("BLITDEBUG:%02X:%06X %04X %04X %04X  %06X %04X %04X %04X  %04X %04X  %02X %02X\n", \
    blitctrl,\
    blitsrca,blitsrcl,blitsrcx,blitsrcy,\
    blitdsta,blitdstl,blitdstx,blitdsty,\
    blitsizx,blitsizy,blitfill1,blitfill2);

  blitmode=blitctrl&0x38;
  pixmode=blitctrl&7;
  blitbackx=blitctrl&D_BLITBACKX;
  blitbacky=blitctrl&D_BLITBACKY;
  pixff=0;
  if ((blitsizx==0)||(blitsizy==0)) return;

  if (blitbacky) {
    for(blitposy=blitsizy-1;blitposy>=0;blitposy--) {
      blitline();
    }
  } else {
    for(blitposy=0;blitposy<blitsizy;blitposy++) {
      blitline();
    };
  };
  if (blitmode==D_BMTEXT) blitdstx+=blitsizx;
};
