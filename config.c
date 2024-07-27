/*
 * This file is part of the SymbosVM project, which is distributed
 * under the terms of the GNU General Public License; either
 * version 2 of the License, or (at your option) any later version.
 */

// written by insane/Rabenauge^.tSCc.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include "simz80.h"
#include "hardware.h"

#define LINEBUFSIZE 1024
#define WMTITLE "SYMBOSVM"

#define CONFIGINI "symbosvm.ini"
#define DEFAULTBIN "symbosvm.bin"

#define _CONFIG_H_IMPLEMENTATION
#include "config.h"

void config_initvars() {
  var_video_vmresx=512;
  var_video_vmresy=256;
  var_video_fullscreen=1;
  var_video_aspect=0;
  var_video_scale2x=1;
  var_audio_enabled=1;
  var_audio_samplerate=48000;
  var_system_z80memdump=0;
  var_system_memdump=0;
  var_system_clionly=0;
  var_system_grabkeys=0;
  var_debug_blit=0;
  var_debug_clock=0;
  var_debug_rtcwrite=0;
  strcpy(var_system_bootfile,DEFAULTBIN);
  strcpy(var_system_hdd_0,"");
  strcpy(var_system_hdd_1,"");
  strcpy(var_system_hdd_2,"");
  strcpy(var_system_hdd_3,"");
  strcpy(var_system_hdd_4,"");
  strcpy(var_system_hdd_5,"");
  strcpy(var_system_hdd_6,"");
  strcpy(var_system_hdd_7,"");
};

void config_parseopt(char *opt,char *val) {
  if (strcmp(opt,"video.vmresx")==0)	 	var_video_vmresx=atoi(val);
  if (strcmp(opt,"video.vmresy")==0)		var_video_vmresy=atoi(val);
  if (strcmp(opt,"video.fullscreen")==0) 	var_video_fullscreen=atoi(val);
  if (strcmp(opt,"video.aspect")==0) 		var_video_aspect=atoi(val);
  if (strcmp(opt,"video.scale2x")==0) 		var_video_scale2x=atoi(val);
  if (strcmp(opt,"audio.enabled")==0) 		var_audio_enabled=atoi(val);
  if (strcmp(opt,"audio.samplerate")==0) 	var_audio_samplerate=atoi(val);
  if (strcmp(opt,"debug.blit")==0)	 	var_debug_blit=atoi(val);
  if (strcmp(opt,"debug.clock")==0)	 	var_debug_clock=atoi(val);
  if (strcmp(opt,"debug.rtcwrite")==0)	 	var_debug_rtcwrite=atoi(val);
  if (strcmp(opt,"system.z80memdump")==0) 	var_system_z80memdump=atoi(val);
  if (strcmp(opt,"system.memdump")==0)	 	var_system_memdump=atoi(val);
  if (strcmp(opt,"system.clionly")==0)	 	var_system_clionly=atoi(val);
  if (strcmp(opt,"system.grabkeys")==0)	 	var_system_grabkeys=atoi(val);
  if (strcmp(opt,"system.bootfile")==0)	 	snprintf(var_system_bootfile,CONFIGSTRINGSIZE-1,"%s",val);
  if (strcmp(opt,"system.hdd.0")==0)	 	snprintf(var_system_hdd_0,CONFIGSTRINGSIZE-1,"%s",val);
  if (strcmp(opt,"system.hdd.1")==0)	 	snprintf(var_system_hdd_1,CONFIGSTRINGSIZE-1,"%s",val);
  if (strcmp(opt,"system.hdd.2")==0)	 	snprintf(var_system_hdd_2,CONFIGSTRINGSIZE-1,"%s",val);
  if (strcmp(opt,"system.hdd.3")==0)	 	snprintf(var_system_hdd_3,CONFIGSTRINGSIZE-1,"%s",val);
  if (strcmp(opt,"system.hdd.4")==0)	 	snprintf(var_system_hdd_4,CONFIGSTRINGSIZE-1,"%s",val);
  if (strcmp(opt,"system.hdd.5")==0)	 	snprintf(var_system_hdd_5,CONFIGSTRINGSIZE-1,"%s",val);
  if (strcmp(opt,"system.hdd.6")==0)	 	snprintf(var_system_hdd_6,CONFIGSTRINGSIZE-1,"%s",val);
  if (strcmp(opt,"system.hdd.7")==0)	 	snprintf(var_system_hdd_7,CONFIGSTRINGSIZE-1,"%s",val);
};

int config_readini() {
  char linebuf[LINEBUFSIZE];
  char workbuf[LINEBUFSIZE];
  char homecfg[LINEBUFSIZE];
  FILE *f=fopen(CONFIGINI,"r");
  if (f==NULL) {
    char *home=getenv("HOME");
    snprintf(homecfg,LINEBUFSIZE,"%s/"CONFIGINI,home);
    f=fopen(homecfg,"r");
  };

  if (f!=NULL) {
    int br;
    do {
      br=fgets(linebuf,LINEBUFSIZE,f)==NULL;
      if (br==0) {
        char *src=linebuf;char *dst=workbuf;
	int iq=0;
        while(*src&&(*src!=';')) { 
	  if (*src=='"') {
	    iq^=1;
	    src++;
	    continue;
	  };
	  if (iq) {
	    *(dst++)=*src;
	  } else {
            if(*src>0x20) *(dst++)=tolower(*src);
	  };
          src++;
        };
        *dst=0;
	if ((src=strchr(workbuf,'='))!=NULL) {
	  *(src++)=0;
	  config_parseopt(workbuf,src);
	};
      };
    } while (br==0);
    fclose(f);

    return 1;
  };
  return 0;
};

