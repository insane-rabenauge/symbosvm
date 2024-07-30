#ifndef _VIDEO_H
#define _VIDEO_H

#include <stdint.h>

#define CHRRAMSIZE (8*256)

extern uint8_t video_chr[];
extern int video_mode;
extern uint16_t video_mptrx;
extern uint16_t video_mptry;
extern uint32_t video_pal[];
extern uint32_t video_ptr;
extern uint8_t video_txtcurx;
extern uint8_t video_txtcury;
extern uint8_t video_cursor[];
extern uint8_t video_font8x8[];
extern uint8_t video_font8x16[];
extern uint8_t video_ptrgfx[];

int init_video();
int done_video();
void video_update();

#endif
