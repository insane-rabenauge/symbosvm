/*
 * This file is part of the SymbosVM project, which is distributed
 * under the terms of the GNU General Public License; either
 * version 2 of the License, or (at your option) any later version.
 */

// written by insane/Rabenauge^.tSCc.

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "system.h"
#include "video.h"
#include "simz80.h"
#include "portio.h"
#include "config.h"

uint32_t video_pal[256];
uint32_t video_ptr;
uint16_t video_mptrx;
uint16_t video_mptry;
uint8_t video_ptrgfx[256];
uint8_t video_ptrptr=0;

int video_mode=0;

uint8_t video_font8x8[256*8];
uint8_t video_font8x16[256*16];

void video_update() {
  if (video_mode==D_VIDTXT8X8) {
    int chrx=var_video_vmresx/8;
    int chry=var_video_vmresy/8;
    for (int ty=0;ty<chry;ty++) {
      for (int tx=0;tx<chrx;tx++) {
	int ch=z80_mem[video_ptr+(ty*256)+tx];

	for (int cy=0;cy<8;cy++) {
	  int vidofs=var_video_vmresx*(ty*8+cy)+(tx*8);
	  int bmp=video_font8x8[ch*8+cy];
	  sys_vidbuf[vidofs++]=(bmp&0x80)?1:0;
	  sys_vidbuf[vidofs++]=(bmp&0x40)?1:0;
	  sys_vidbuf[vidofs++]=(bmp&0x20)?1:0;
	  sys_vidbuf[vidofs++]=(bmp&0x10)?1:0;
	  sys_vidbuf[vidofs++]=(bmp&0x08)?1:0;
	  sys_vidbuf[vidofs++]=(bmp&0x04)?1:0;
	};
      };
    };
  } else if (video_mode==D_VIDTXT8X16) {
    int chrx=var_video_vmresx/8;
    int chry=var_video_vmresy/16;
    for (int ty=0;ty<chry;ty++) {
      for (int tx=0;tx<chrx;tx++) {
	int ch=z80_mem[video_ptr+(ty*256)+tx];

	for (int cy=0;cy<16;cy++) {
	  int vidofs=var_video_vmresx*(ty*16+cy)+(tx*8);
	  int bmp=video_font8x16[ch*16+cy];
	  sys_vidbuf[vidofs++]=(bmp&0x80)?1:0;
	  sys_vidbuf[vidofs++]=(bmp&0x40)?1:0;
	  sys_vidbuf[vidofs++]=(bmp&0x20)?1:0;
	  sys_vidbuf[vidofs++]=(bmp&0x10)?1:0;
	  sys_vidbuf[vidofs++]=(bmp&0x08)?1:0;
	  sys_vidbuf[vidofs++]=(bmp&0x04)?1:0;
	};
      };
    };
  } else if (video_mode==D_VIDGFX4BPP) {
    int vidrd=video_ptr;
    int vidwr=0;
    for (int py=0;py<var_video_vmresy;py++) {
      for (int px=0;px<var_video_vmresx/2;px++) {
	int pix=z80_mem[vidrd];
	sys_vidbuf[vidwr+0]=(pix>>4)&15;
	sys_vidbuf[vidwr+1]=(pix>>0)&15;
	vidrd+=1;vidwr+=2;
      };
    };
  } else if (video_mode==D_VIDGFX8BPP) {
    memcpy(sys_vidbuf,&z80_mem[video_ptr],var_video_vmresx*var_video_vmresy);
  } else if (video_mode==D_VIDGFX2CPC) {
    int vidrd;int vidwr;
    int vidofsx=(var_video_vmresx-(320))/2;
    int vidofsy=(var_video_vmresy-(200))/2;
    if (var_video_vmresx<320) return;
    if (var_video_vmresy<200) return;
    for (int ly=0;ly<200;ly++) {
      vidrd=video_ptr+((ly>>3)*80)+((ly&7)<<11);
      vidwr=var_video_vmresx*(vidofsy+ly)+vidofsx;
      for (int lx=0;lx<320/4;lx++) {
	int pix=z80_mem[vidrd++];
	sys_vidbuf[vidwr++]=((pix&0x80)>>7)|((pix&0x08)>>2);
	sys_vidbuf[vidwr++]=((pix&0x40)>>6)|((pix&0x04)>>1);
	sys_vidbuf[vidwr++]=((pix&0x20)>>5)|((pix&0x02)   );
	sys_vidbuf[vidwr++]=((pix&0x10)>>4)|((pix&0x01)<<1);
      };
    };
  } else {
    memset(sys_vidbuf,0,var_video_vmresx*var_video_vmresy);
  };

  if (sys_mouseon) {
    for (int y=0;y<16;y++) {
      for (int x=0;x<16;x++) {
	int drwx=video_mptrx+x;
	int drwy=video_mptry+y;
	if ((drwy<var_video_vmresy)&&(drwy>=0)) {
	  if ((drwx<var_video_vmresx)&&(drwx>=0)) {
	    int8_t mp=video_ptrgfx[y*16+x];
	    if (mp>=0) sys_vidbuf[drwy*var_video_vmresx+drwx]=mp;
	  };
	};
      };
    };
  };
  system_blit(sys_vidbuf,video_pal);
};

int init_video() {
  for (int i=0;i<256;i++) video_pal[i]=0;
  video_pal[0]=0xaaaaaa;
  video_pal[1]=0x000000;
  video_mode=0; // TEXT
  video_ptr=0x010000;
  return 1;
};

int done_video() {
  return 1;
};
