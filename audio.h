#ifndef _AUDIO_H
#define _AUDIO_H

void audio_beep(uint8_t freq);
void audio_update();
void audio_reset();
int init_audio();
int done_audio();

#endif
