#ifndef _CONFIG_H
#define _CONFIG_H

#ifndef _CONFIG_H_IMPLEMENTATION
void config_initvars();
int config_readini();
#define _EXTERN extern
#define CONFIGSTRINGSIZE
#else
#define _EXTERN
#define CONFIGSTRINGSIZE LINEBUFSIZE
#endif

_EXTERN int var_video_vmresx;
_EXTERN int var_video_vmresy;
_EXTERN int var_video_fullscreen;
_EXTERN int var_video_aspect;
_EXTERN int var_video_scaler;
_EXTERN int var_video_scale2x;
_EXTERN int var_audio_enabled;
_EXTERN int var_audio_samplerate;
_EXTERN int var_debug_blit;
_EXTERN int var_debug_clock;
_EXTERN int var_debug_rtcwrite;
_EXTERN int var_system_z80memdump;
_EXTERN int var_system_memdump;
_EXTERN int var_system_clionly;
_EXTERN int var_system_grabkeys;
_EXTERN char var_system_bootfile[CONFIGSTRINGSIZE];
_EXTERN char var_system_hdd_0[CONFIGSTRINGSIZE];
_EXTERN char var_system_hdd_1[CONFIGSTRINGSIZE];
_EXTERN char var_system_hdd_2[CONFIGSTRINGSIZE];
_EXTERN char var_system_hdd_3[CONFIGSTRINGSIZE];
_EXTERN char var_system_hdd_4[CONFIGSTRINGSIZE];
_EXTERN char var_system_hdd_5[CONFIGSTRINGSIZE];
_EXTERN char var_system_hdd_6[CONFIGSTRINGSIZE];
_EXTERN char var_system_hdd_7[CONFIGSTRINGSIZE];

#undef _EXTERN
#endif
