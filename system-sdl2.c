/*
 * This file is part of the SymbosVM project, which is distributed
 * under the terms of the GNU General Public License; either
 * version 2 of the License, or (at your option) any later version.
 */

// written by insane/Rabenauge^.tSCc.

#include <SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "simz80.h"
#include "config.h"
#include "hardware.h"
#include "rtc.h"

#define WMTITLE "SYMBOSVM"

SDL_TimerID timerID;

volatile int sys_timer_irq=0;
const int sys_timer_freq=50;
const int sys_timer_vector=0x38;

int sys_quit = 0;
int sys_quit_type = D_VMEXIT;

SDL_Window *sdlwindow;
SDL_Texture *sdltexture;
SDL_Event sdlevent;
SDL_Renderer *sdlrenderer;

SDL_mutex *sdlsynclock=NULL;
SDL_cond *sdlsynccond=NULL;

int sys_default_vmresx;
int sys_default_vmresy;
int sys_scale2x=0;
int screenx=0;
int screeny=0;
int8_t sys_mousex=0;
int8_t sys_mousey=0;
uint8_t sys_mouseb=0;
int sys_mouseon=0;
int mousex=0;
int mousey=0;
int mousewx=0;
int mousewy=0;

int getkey=0;

char keystate[SDL_NUM_SCANCODES];

uint8_t* sys_vidbuf;
SDL_Rect screen_rect;

uint32_t system_gettickus() {
  uint64_t sdl_us=SDL_GetPerformanceCounter()/(SDL_GetPerformanceFrequency()/1000000);
  return sdl_us;
};

void system_rtcload(void) {
  time_t t = time(NULL);
  struct tm tm = *localtime(&t);
  int y=tm.tm_year+1900;
  rtc_rtcdata[D_RTCYLO]=y&0xff;rtc_rtcdata[D_RTCYHI]=y>>8;
  rtc_rtcdata[D_RTCMON]=tm.tm_mon+1;
  rtc_rtcdata[D_RTCDAY]=tm.tm_mday;
  rtc_rtcdata[D_RTCHOU]=tm.tm_hour;
  rtc_rtcdata[D_RTCMIN]=tm.tm_min;
  rtc_rtcdata[D_RTCSEC]=tm.tm_sec;

  if (var_debug_clock) {
    printf("RTCDEBUG: LOAD: ");
    printf("%02i.%02i.%04i %02i:%02i:%02i\n",
      rtc_rtcdata[D_RTCDAY],
      rtc_rtcdata[D_RTCMON],
      rtc_rtcdata[D_RTCYLO]|(rtc_rtcdata[D_RTCYHI]<<8),
      rtc_rtcdata[D_RTCHOU],
      rtc_rtcdata[D_RTCMIN],
      rtc_rtcdata[D_RTCSEC]
    );
  };
};

void system_rtcsave(void) {
  if (var_debug_clock) {
    printf("RTCDEBUG: SAVE: ");
    printf("%02i.%02i.%04i %02i:%02i:%02i\n",
      rtc_rtcdata[D_RTCDAY],
      rtc_rtcdata[D_RTCMON],
      rtc_rtcdata[D_RTCYLO]|(rtc_rtcdata[D_RTCYHI]<<8),
      rtc_rtcdata[D_RTCHOU],
      rtc_rtcdata[D_RTCMIN],
      rtc_rtcdata[D_RTCSEC]
    );
  };
};

void system_initscan(int key) {
  getkey=(key==0)?1:key;
};

int system_scankey() {
  int ch=0;
  for (int i=getkey;i<255;i++) {
    if(keystate[i]) {
      ch=i;
      getkey=i+1; // next key to check
      break;
    };
  };
  return ch;
};

int system_setres(int x,int y) {
  var_video_vmresx=x;
  var_video_vmresy=y;

  sys_scale2x=var_video_scale2x;
  if ((x>=1024)||(y>=1024)) sys_scale2x=0;

  if (sys_scale2x) {
    screenx=2*var_video_vmresx;
    screeny=2*var_video_vmresy;
  } else {
    screenx=var_video_vmresx;
    screeny=var_video_vmresy;
  };

  screen_rect.x=screen_rect.y=0;
  screen_rect.w=screenx;screen_rect.h=screeny;

  if(!sdlwindow) {
    sdlwindow=SDL_CreateWindow(WMTITLE,SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,screenx,screeny,0);
    if (sdlwindow==NULL) {
      printf("SDL_CreateWindow Error: %s\n", SDL_GetError() );
      return 0;
    };
  };

  if(!sdlrenderer) {
    sdlrenderer=SDL_CreateRenderer(sdlwindow,-1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_TARGETTEXTURE);
    if (sdlrenderer==NULL) {
      printf("SDL_CreateRenderer Error: %s\n", SDL_GetError() );
      return 0;
    };
  };

  SDL_SetRenderDrawBlendMode(sdlrenderer, SDL_BLENDMODE_NONE);

  if (sys_vidbuf) free(sys_vidbuf);
  sys_vidbuf=calloc(var_video_vmresx*var_video_vmresy,1);

  SDL_SetWindowSize(sdlwindow,screenx,screeny);

  if(sdltexture) SDL_DestroyTexture(sdltexture);

  sdltexture=SDL_CreateTexture(sdlrenderer,SDL_PIXELFORMAT_ARGB8888,SDL_TEXTUREACCESS_STREAMING,screenx,screeny);
  if (sdltexture==NULL) {
    printf("SDL_CreateTexture Error: %s\n", SDL_GetError() );
    return 0;
  };

  SDL_SetTextureBlendMode(sdltexture, SDL_BLENDMODE_NONE);

  if (var_video_aspect) {
    SDL_RenderSetLogicalSize(sdlrenderer,screenx,screeny);
  } else {
    SDL_RenderSetLogicalSize(sdlrenderer,0,0);
  };
  SDL_SetWindowPosition(sdlwindow,SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED);
  return 1;
};

void system_setfullscreen() {
  if (var_video_fullscreen) {
    SDL_SetWindowFullscreen(sdlwindow,SDL_WINDOW_FULLSCREEN_DESKTOP);
  } else {
    SDL_SetWindowFullscreen(sdlwindow,0);
  };
};

void system_event_handler() {
  while(SDL_PollEvent(&sdlevent)) {
    switch (sdlevent.type) {
      case SDL_QUIT:
        sys_quit = 1;break;
      case SDL_MOUSEWHEEL:
        mousewx=sdlevent.wheel.x;
        mousewy=sdlevent.wheel.y;
        // ignore FLIPPED
        break;
      case SDL_KEYUP:
        keystate[sdlevent.key.keysym.scancode]=0;
        break;
      case SDL_KEYDOWN:
        keystate[sdlevent.key.keysym.scancode]=1;
        if (keystate[SDL_SCANCODE_LCTRL]&&keystate[SDL_SCANCODE_LALT]&&keystate[SDL_SCANCODE_C]) sys_quit=1;
        if (keystate[SDL_SCANCODE_LCTRL]&&keystate[SDL_SCANCODE_LALT]&&keystate[SDL_SCANCODE_RETURN]) {
          var_video_fullscreen^=1;
          system_setfullscreen();
        };
        break;
    }
  }
}

void system_blit(uint8_t* screen, uint32_t* pal) {
  uint32_t* sys_pixbuf;
  int pitch,pitchx;

  int sptr=0;int dptr=0;int dofs=0;int c=0;

  SDL_LockTexture(sdltexture, NULL, (void**)&sys_pixbuf, &pitch);
  pitchx=pitch/4;
  if (sys_scale2x) {
    for (int y=0;y<var_video_vmresy;y++) {
      dptr=dofs;
      for (int x=0;x<var_video_vmresx;x++) {
        c=pal[screen[sptr++]];
        sys_pixbuf[dptr+pitchx]=c;
        sys_pixbuf[dptr++]=c;
        sys_pixbuf[dptr+pitchx]=c;
        sys_pixbuf[dptr++]=c;
      };
      dofs+=2*pitchx;
    };
  } else {
    for (int y=0;y<var_video_vmresy;y++) {
      dptr=dofs;
      for (int x=0;x<var_video_vmresx;x++) {
        sys_pixbuf[dptr++]=pal[screen[sptr++]];
      };
      dofs+=pitchx;
    };
  };

  SDL_UnlockTexture(sdltexture);
  SDL_RenderCopy(sdlrenderer,sdltexture,&screen_rect,NULL);
  SDL_RenderPresent(sdlrenderer);
};

void system_update() {
  system_event_handler();
};

void system_reset() {
  system_setres(var_video_vmresx,var_video_vmresy);
};

void system_update_mouse() {
  int sdl_mouseb=SDL_GetRelativeMouseState(&mousex,&mousey);
  sys_mouseb=0;
  if (sdl_mouseb&SDL_BUTTON(SDL_BUTTON_LEFT))   sys_mouseb|=D_MBLEFT;
  if (sdl_mouseb&SDL_BUTTON(SDL_BUTTON_RIGHT))  sys_mouseb|=D_MBRIGHT;
  if (sdl_mouseb&SDL_BUTTON(SDL_BUTTON_MIDDLE)) sys_mouseb|=D_MBMIDDLE;

  if (mousewy==1)  sys_mouseb|=D_MBWHEELUP;
  if (mousewy==-1) sys_mouseb|=D_MBWHEELDOWN;
  mousewx=mousewy=0;
  sys_mousex=mousex/2;if(sys_mousex>127)sys_mousex=127;if(sys_mousex<-128)sys_mousex=-128;
  sys_mousey=mousey/2;if(sys_mousey>127)sys_mousey=127;if(sys_mousey<-128)sys_mousey=-128;
};

void system_capturemouse(int capture) {
  sys_mouseon=capture;
  SDL_SetRelativeMouseMode(capture);
};

void system_waitfortimer() {
  SDL_LockMutex(sdlsynclock);
  if (!sys_timer_irq) {
    SDL_CondWait(sdlsynccond,sdlsynclock);
  };
  SDL_UnlockMutex(sdlsynclock);
};

void system_timersignal() {
  SDL_CondSignal(sdlsynccond);
};

uint32_t system_timercallback(uint32_t interval, void* param ) {
  z80_running=0;sys_timer_irq=1;
  system_timersignal();
  return interval;
}

int system_initsdl() {
  if (SDL_Init(SDL_INIT_VIDEO|SDL_INIT_AUDIO|SDL_INIT_TIMER)<0) {
    printf("SDL Initialization Error: %s\n", SDL_GetError() );
    return 0;
  };

  SDL_DisplayMode dm;
  if (SDL_GetDesktopDisplayMode(0,&dm)==0) {
    if (!var_video_vmresx) var_video_vmresx=dm.w;
    if (!var_video_vmresy) var_video_vmresy=dm.h;
  };

  if (!var_video_vmresx) var_video_vmresx=800; // safe defaults
  if (!var_video_vmresy) var_video_vmresy=600; // safe defaults

  sys_default_vmresx=var_video_vmresx;
  sys_default_vmresy=var_video_vmresy;

  SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");

#if SDL_PATCHLEVEL >= 1
  SDL_SetHint(SDL_HINT_VIDEO_HIGHDPI_DISABLED, "1");
#endif

  if (!system_setres(var_video_vmresx,var_video_vmresy)) return 0;

  system_setfullscreen();

  SDL_SetRelativeMouseMode(0);

#if SDL_VERSION_ATLEAST(2,0,16)
  SDL_SetWindowKeyboardGrab(sdlwindow,var_system_grabkeys);
#endif

  srand(time(NULL));
  sdlsynclock=SDL_CreateMutex();
  sdlsynccond=SDL_CreateCond();

  timerID=SDL_AddTimer(1000/sys_timer_freq,system_timercallback,NULL);
  return 1;
};

void system_donesdl() {
  SDL_RemoveTimer(timerID);
  SDL_DestroyMutex(sdlsynclock);sdlsynclock=NULL;
  SDL_DestroyCond(sdlsynccond);sdlsynccond=NULL;
  SDL_SetWindowFullscreen(sdlwindow,0);
  SDL_SetRelativeMouseMode(0);
  SDL_DestroyTexture(sdltexture);
  SDL_DestroyRenderer(sdlrenderer);
  SDL_DestroyWindow(sdlwindow);
  SDL_Quit();
};

void preinit_system() {
  config_initvars();
  config_readini();
  rtc_rtcstat=D_RTCACT;
  if (var_debug_rtcwrite) rtc_rtcstat|=D_RTCWRT;
};

int init_system() {
  int retval=1;
  retval&=system_initsdl();
  return retval;
};

int done_system() {
  system_donesdl();

  if (var_system_z80memdump) {
    FILE *f=fopen("MEMDUMP.Z80","wb");
    for (int i=0;i<4;i++) fwrite(z80_bank[i],BANK_SIZE,1,f);
    fclose(f);

  };
  if (var_system_memdump) {
    FILE *f=fopen("MEMDUMP.BIN","wb");
    int maxmem=MEM_SIZE-1;
    while(z80_mem[maxmem]==0)--maxmem;
    fwrite(z80_mem,maxmem,1,f);
    fclose(f);
  };
  return 1;
};
