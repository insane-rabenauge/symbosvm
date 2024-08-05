#ifndef _BLITTER_H
#define _BLITTER_H

void startblit(uint8_t blitctrl);

extern uint32_t blitsrca;
extern uint16_t blitsrcl;
extern uint16_t blitsrcx;
extern uint16_t blitsrcy;
extern uint32_t blitdsta;
extern uint16_t blitdstl;
extern uint16_t blitdstx;
extern uint16_t blitdsty;
extern uint16_t blitsizx;
extern uint16_t blitsizy;
extern uint8_t blitfill1;
extern uint8_t blitfill2;

#endif
