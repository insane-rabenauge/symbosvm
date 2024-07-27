/*
 * This file is part of the SymbosVM project, which is distributed
 * under the terms of the GNU General Public License; either
 * version 2 of the License, or (at your option) any later version.
 */

// written by insane/Rabenauge^.tSCc.

#include <stdio.h>
#include <string.h>
#include "simz80.h"
#include "portio.h"
#include "video.h"
#include "audio.h"
#include "vmz80.h"
#include "system.h"
#include "config.h"
#include "drives.h"
#include "blitter.h"

uint32_t memptr1=0,memptr2=0,dmaptr=0,dsklba=0;
int memdma=0;
uint8_t	palsel=0,seccnt=0,rtcsel=0;
int chrptr=0,fntsel=0,dskstat=0,drive=0;

void printpc(unsigned int PC) {
  int pcb=(simz80_getbank((PC>>14)&3))*0x4000;
  int pco=PC&0x3fff;
  int pc24=pcb|pco;
  printf("PC=%06X ",pc24);
  int addr=pc24-2;
  printf("[%06X]=",addr);
  for (int i=0;i<6;i++) {
    printf("%02X ",z80_mem[addr++]);
  };
  printf("\n");
};

int z80_in(unsigned int port, unsigned int PC) {
  switch (port&0xff) {
    case P_BANK0:
      return simz80_getbank(0);
      break;
    case P_BANK1:
      return simz80_getbank(1);
      break;
    case P_BANK2:
      return simz80_getbank(2);
      break;
    case P_BANK3:
      return simz80_getbank(3);
      break;
    case P_MEMPTR1_L:
      return(memptr1&0xff);
      break;
    case P_MEMPTR1_H:
      return((memptr1>>8)&0xff);
      break;
    case P_MEMPTR1_U:
      return((memptr1>>16)&MEM_MASKU);
      break;
    case P_MEMACC1DB:
      return(z80_mem[--memptr1]);
      break;
    case P_MEMACC1:
      return(z80_mem[memptr1]);
      break;
    case P_MEMACC1IA:
      return(z80_mem[memptr1++]);
      break;
    case P_MEMPTR2_L:
      return(memptr2&0xff);
      break;
    case P_MEMPTR2_H:
      return((memptr2>>8)&0xff);
      break;
    case P_MEMPTR2_U:
      return((memptr2>>16)&MEM_MASKU);
      break;
    case P_MEMACC2DB:
      return(z80_mem[--memptr2]);
      break;
    case P_MEMACC2:
      return(z80_mem[memptr2]);
      break;
    case P_MEMACC2IA:
      return(z80_mem[memptr2++]);
      break;
    case P_KEYSCAN:
      return(system_scankey());
      break;
    case P_USEDCPU:
      return(z80_usage);
      break;
    case P_RTCCTRL:
      return(system_rtcstat);
      break;
    case P_RTCDATA:
      if(rtcsel>7)rtcsel=0;
      return(system_rtcdata[rtcsel++]);
      break;
    case P_MOUSEX:
      return(sys_mousex);
      break;
    case P_MOUSEY:
      return(sys_mousey);
      break;
    case P_MOUSEB:
      return(sys_mouseb);
      break;
    case P_DSKPTR_L:
      return(dmaptr&0xff);
      break;
    case P_DSKPTR_H:
      return((dmaptr>>8)&0xff);
      break;
    case P_DSKPTR_U:
      return((dmaptr>>16)&MEM_MASKU);
      break;
    case P_DSKCNT:
      return(seccnt&0xff);
      break;
    case P_DSKLBA_0:
      return(dsklba&0xff);
      break;
    case P_DSKLBA_1:
      return((dsklba>>8)&0xff);
      break;
    case P_DSKLBA_2:
      return((dsklba>>16)&0xff);
      break;
    case P_DSKLBA_3:
      return((dsklba>>24)&0xff);
      break;
    case P_DSKCMD:
      return(dskstat&0xff);
      break;
    case P_VIDPTR_L:
      return(video_ptr&0xff);
      break;
    case P_VIDPTR_H:
      return((video_ptr>>8)&0xff);
      break;
    case P_VIDPTR_U:
      return((video_ptr>>16)&MEM_MASKU);
      break;
    case P_VIDRESX_L:
      return(var_video_vmresx&0xff);
      break;
    case P_VIDRESX_H:
      return(var_video_vmresx>>8)&0xff;
      break;
    case P_VIDRESY_L:
      return(var_video_vmresy)&0xff;
      break;
    case P_VIDRESY_H:
      return(var_video_vmresy>>8)&0xff;
      break;
    case P_VIDMODE:
      return(video_mode);
      break;
    case P_PALSEL:
      return(palsel);
      break;
    case P_PALR:
      return((video_pal[palsel]>>16)&0xff);
      break;
    case P_PALG:
      return((video_pal[palsel]>>8 )&0xff);
      break;
    case P_PALB:
      return((video_pal[palsel]>>0 )&0xff);
      break;
    case P_BLITCTRL:
      return 0;
      break;
    default:
      printf("%s: %04X, ",__func__,port);
      printpc(PC);
  };
  return(0xff);
};

void z80_out(unsigned int port, unsigned int value, unsigned int PC) {
  switch (port&0xff) {
    case P_BANK0:
      simz80_setbank(0,value);
      break;
    case P_BANK1:
      simz80_setbank(1,value);
      break;
    case P_BANK2:
      simz80_setbank(2,value);
      break;
    case P_BANK3:
      simz80_setbank(3,value);
      break;
    case P_MEMPTR1_L:
      memptr1=(memptr1&MEM_MASK&0xffff00)|value;
      break;
    case P_MEMPTR1_H:
      memptr1=(memptr1&MEM_MASK&0xff00ff)|(value<<8);
      break;
    case P_MEMPTR1_U:
      memptr1=(memptr1&MEM_MASK&0x00ffff)|((value&MEM_MASKU)<<16);
      break;
    case P_MEMACC1DB:
      z80_mem[--memptr1]=value;
      break;
    case P_MEMACC1:
      z80_mem[memptr1]=value;
      break;
    case P_MEMACC1IA:
      z80_mem[memptr1++]=value;
      break;
    case P_MEMPTR2_L:
      memptr2=(memptr2&MEM_MASK&0xffff00)|value;
      break;
    case P_MEMPTR2_H:
      memptr2=(memptr2&MEM_MASK&0xff00ff)|(value<<8);
      break;
    case P_MEMPTR2_U:
      memptr2=(memptr2&MEM_MASK&0x00ffff)|((value&MEM_MASKU)<<16);
      break;
    case P_MEMACC2DB:
      z80_mem[--memptr2]=value;
      break;
    case P_MEMACC2:
      z80_mem[memptr2]=value;
      break;
    case P_MEMACC2IA:
      z80_mem[memptr2++]=value;
      break;
    case P_EXIT:
      puts("");
      z80_run=0;sys_quit=1;
      break;
    case P_CONOUT:
      putchar(value);
      fflush(stdout);
      break;
    case P_MEMDMA_L:
      memdma=(memdma&0xff00)|value;
      break;
    case P_MEMDMA_H:
      memdma=(memdma&0xff)|(value<<8);
      memdma&=0xffff; if (memdma==0) memdma=0x10000;
      memcpy(&z80_mem[memptr2],&z80_mem[memptr1],memdma);
      break;
    case P_DEBOUT:
      printf("DEBUG VALUE #%02X\n",value);
      fflush(stdout);
      break;
    case P_BEEP:
      audio_beep(value);
      break;
    case P_KEYSCAN:
      system_initscan(value);
      break;
    case P_RTCCTRL:
      switch (value&3) {
	case D_RTCREAD:
	  rtcsel=0;
	  break;
	case 1:
	  break;
	case D_RTCLOAD:
	  system_rtcload();
	  break;
	case D_RTCSAVE:
	  system_rtcsave();
	  break;
      };
      break;
    case P_RTCDATA:
      if(rtcsel>7)rtcsel=0;
      system_rtcdata[rtcsel++]=value;
      break;
    case P_MOUSECTRL:
      switch (value&3) {
	case D_MOUSEUPDATE:
          system_update_mouse();
	  break;
	case 1:
	  break;
	case D_MOUSEOFF:
	  system_capturemouse(0);
	  break;
	case D_MOUSEON:
	  system_capturemouse(1);
	  break;
      };
      break;
    case P_MPTRX_LO:
      video_mptrx=(video_mptrx&0xff00)|value;
      break;
    case P_MPTRX_HI:
      video_mptrx=(video_mptrx&0xff)|(value<<8);
      break;
    case P_MPTRY_LO:
      video_mptry=(video_mptry&0xff00)|value;
      break;
    case P_MPTRY_HI:
      video_mptry=(video_mptry&0xff)|(value<<8);
      break;
    case P_MSPRSEL:
      video_ptrptr=value;
      break;
    case P_MSPRDAT:
      video_ptrgfx[video_ptrptr++]=value;
      break;
    case P_DSKPTR_L:
      dmaptr=(dmaptr&MEM_MASK&0xffff00)|value;
      break;
    case P_DSKPTR_H:
      dmaptr=(dmaptr&MEM_MASK&0xff00ff)|(value<<8);
      break;
    case P_DSKPTR_U:
      dmaptr=(dmaptr&MEM_MASK&0x00ffff)|((value&MEM_MASKU)<<16);
      break;
    case P_DSKCNT:
      seccnt=value;
      break;
    case P_DSKLBA_0:
      dsklba=(dsklba&0xffffff00)|value;
      break;
    case P_DSKLBA_1:
      dsklba=(dsklba&0xffff00ff)|(value<<8);
      break;
    case P_DSKLBA_2:
      dsklba=(dsklba&0xff00ffff)|(value<<16);
      break;
    case P_DSKLBA_3:
      dsklba=(dsklba&0x00ffffff)|(value<<24);
      break;
    case P_DSKCMD:
      drive=value&7;
      switch (value&0b11000) {
	case D_DSKINFO:
	  dskstat=drive_getinfo(drive,&dsklba);
	  break;
	case D_DSKREAD:
	  dskstat=drive_read(drive,seccnt,dsklba,dmaptr);
	  break;
	case D_DSKWRITE:
	  dskstat=drive_write(drive,seccnt,dsklba,dmaptr);
	  break;
      };
      break;
    case P_VIDPTR_L:
      video_ptr=(video_ptr&MEM_MASK&0xffff00)|value;
      break;
    case P_VIDPTR_H:
      video_ptr=(video_ptr&MEM_MASK&0xff00ff)|(value<<8);
      break;
    case P_VIDPTR_U:
      video_ptr=(video_ptr&MEM_MASK&0x00ffff)|((value&MEM_MASKU)<<16);
      break;
    case P_VIDRESX_L:
      var_video_vmresx=(var_video_vmresx&0xff00)|value;
      break;
    case P_VIDRESX_H:
      var_video_vmresx=(var_video_vmresx&0xff)|(value<<8);
      break;
    case P_VIDRESY_L:
      var_video_vmresy=(var_video_vmresy&0xff00)|value;
      break;
    case P_VIDRESY_H:
      var_video_vmresy=(var_video_vmresy&0xff)|(value<<8);
      break;
    case P_VIDMODE:
      video_mode=value;
      system_setres(var_video_vmresx,var_video_vmresy);
      break;
    case P_CHRSEL:
      chrptr=0;
      fntsel=value;
      break;
    case P_CHRDAT:
      if (fntsel==0) {
        chrptr&=((256*8)-1);
	video_font8x8[chrptr++]=value;
      } else if (fntsel==1) {
        chrptr&=((256*16)-1);
	video_font8x16[chrptr++]=value;
      };
      break;
    case P_PALSEL:
      palsel=value;
      break;
    case P_PALR:
      video_pal[palsel]=(video_pal[palsel]&0x00ffff)|(value<<16);
      break;
    case P_PALG:
      video_pal[palsel]=(video_pal[palsel]&0xff00ff)|(value<<8 );
      break;
    case P_PALB:
      video_pal[palsel]=(video_pal[palsel]&0xffff00)|(value<<0 );
      break;
    case P_BLITSRCA_L:
      blitsrca=(blitsrca&MEM_MASK&0xffff00)|value;
      break;
    case P_BLITSRCA_H:
      blitsrca=(blitsrca&MEM_MASK&0xff00ff)|(value<<8);
      break;
    case P_BLITSRCA_U:
      blitsrca=(blitsrca&MEM_MASK&0x00ffff)|((value&MEM_MASKU)<<16);
      break;
    case P_BLITSRCL_L:
      blitsrcl=(blitsrcl&0xff00)|value;
      break;
    case P_BLITSRCL_H:
      blitsrcl=(blitsrcl&0x00ff)|(value<<8);
      break;
    case P_BLITSRCX_L:
      blitsrcx=(blitsrcx&0xff00)|value;
      break;
    case P_BLITSRCX_H:
      blitsrcx=(blitsrcx&0x00ff)|(value<<8);
      break;
    case P_BLITSRCY_L:
      blitsrcy=(blitsrcy&0xff00)|value;
      break;
    case P_BLITSRCY_H:
      blitsrcy=(blitsrcy&0x00ff)|(value<<8);
      break;
    case P_BLITDSTA_L:
      blitdsta=(blitdsta&MEM_MASK&0xffff00)|value;
      break;
    case P_BLITDSTA_H:
      blitdsta=(blitdsta&MEM_MASK&0xff00ff)|(value<<8);
      break;
    case P_BLITDSTA_U:
      blitdsta=(blitdsta&MEM_MASK&0x00ffff)|((value&MEM_MASKU)<<16);
      break;
    case P_BLITDSTL_L:
      blitdstl=(blitdstl&0xff00)|value;
      break;
    case P_BLITDSTL_H:
      blitdstl=(blitdstl&0x00ff)|(value<<8);
      break;
    case P_BLITDSTX_L:
      blitdstx=(blitdstx&0xff00)|value;
      break;
    case P_BLITDSTX_H:
      blitdstx=(blitdstx&0x00ff)|(value<<8);
      break;
    case P_BLITDSTY_L:
      blitdsty=(blitdsty&0xff00)|value;
      break;
    case P_BLITDSTY_H:
      blitdsty=(blitdsty&0x00ff)|(value<<8);
      break;
    case P_BLITSIZX_L:
      blitsizx=(blitsizx&0xff00)|value;
      break;
    case P_BLITSIZX_H:
      blitsizx=(blitsizx&0x00ff)|(value<<8);
      break;
    case P_BLITSIZY_L:
      blitsizy=(blitsizy&0xff00)|value;
      break;
    case P_BLITSIZY_H:
      blitsizy=(blitsizy&0x00ff)|(value<<8);
      break;
    case P_BLITFILL:
      blitfill=value;
      break;
    case P_BLITCTRL:
      startblit(value);
      break;
    default:
      printf("%s: %04X %02X, ",__func__,port,value);
      printpc(PC);
  };
};

