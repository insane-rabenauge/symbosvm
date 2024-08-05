#ifndef _SYSTEM_H
#define _SYSTEM_H

int init_system();
int done_system();

void preinit_system();

void system_update();
void system_update_mouse();
void system_capturemouse(int capture);
void system_blit(uint8_t* screen, uint32_t* pal);
void system_setres(int x,int y);

void system_initscan(int key);
int system_scankey();

void system_waitfortimer();

void system_rtcload();
void system_rtcsave();

uint32_t system_gettickus();

extern volatile int sys_timer_irq;
extern int sys_quit;
extern const int sys_timer_freq;
extern const int sys_timer_vector;

extern uint32_t* sys_pixbuf;
extern uint8_t* sys_vidbuf;

extern int8_t sys_mousex;
extern int8_t sys_mousey;
extern uint8_t sys_mouseb;
extern int sys_mouseon;

#endif
