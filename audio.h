#ifndef _AUDIO_H
#define _AUDIO_H

void audio_beep(uint8_t freq);
int audio_update();
void audio_reset();
int init_audio();
int done_audio();
int audio_psg1get(int sel);
int audio_psg2get(int sel);
void audio_psg1set(int sel,int dat);
void audio_psg2set(int sel,int dat);
void audio_psg1ctrl(int dat);
void audio_psg2ctrl(int dat);
#endif
