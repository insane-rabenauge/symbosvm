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
_EXTERN int var_audio_buffersize;
_EXTERN int var_debug_blit;
_EXTERN int var_debug_clock;
_EXTERN int var_debug_rtcwrite;
_EXTERN int var_debug_hostfs;
_EXTERN int var_system_z80memdump;
_EXTERN int var_system_memdump;
_EXTERN int var_system_grabkeys;
_EXTERN char var_system_shutdown[CONFIGSTRINGSIZE];
_EXTERN char var_system_bootfile[CONFIGSTRINGSIZE];
_EXTERN char var_mount_folder_0[CONFIGSTRINGSIZE];
_EXTERN char var_mount_folder_1[CONFIGSTRINGSIZE];
_EXTERN char var_mount_folder_2[CONFIGSTRINGSIZE];
_EXTERN char var_mount_folder_3[CONFIGSTRINGSIZE];
_EXTERN char var_mount_folder_4[CONFIGSTRINGSIZE];
_EXTERN char var_mount_folder_5[CONFIGSTRINGSIZE];
_EXTERN char var_mount_folder_6[CONFIGSTRINGSIZE];
_EXTERN char var_mount_folder_7[CONFIGSTRINGSIZE];
_EXTERN char var_mount_image_0[CONFIGSTRINGSIZE];
_EXTERN char var_mount_image_1[CONFIGSTRINGSIZE];
_EXTERN char var_mount_image_2[CONFIGSTRINGSIZE];
_EXTERN char var_mount_image_3[CONFIGSTRINGSIZE];
_EXTERN char var_mount_image_4[CONFIGSTRINGSIZE];
_EXTERN char var_mount_image_5[CONFIGSTRINGSIZE];
_EXTERN char var_mount_image_6[CONFIGSTRINGSIZE];
_EXTERN char var_mount_image_7[CONFIGSTRINGSIZE];
/*
mount.folder
mount.image
*/
#undef _EXTERN
#endif
