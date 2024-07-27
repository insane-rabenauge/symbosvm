/*
 * This file is part of the SymbosVM project, which is distributed
 * under the terms of the GNU General Public License; either
 * version 2 of the License, or (at your option) any later version.
 */

// written by insane/Rabenauge^.tSCc.

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include "simz80.h"
#include "portio.h"
#include "audio.h"
#include "video.h"
#include "vmz80.h"
#include "system.h"
#include "drives.h"
#include "config.h"
#include "release.h"

int doload(uint8_t* buf, char * fnam) {
  FILE *f;
  if ((f=fopen(fnam,"rb"))==NULL) {
    printf("error loading file %s\n",fnam);
    return(0);
  };
  fseek(f,0,SEEK_END);
  int fsiz=ftell(f);
  rewind(f);
  if (fsiz>=MEM_SIZE) {
    printf("not enough memory\n");
    fclose(f);
    return(0);
  };
  fread(buf,fsiz,1,f);
  fclose(f);
  return(1);
};

int main(int argc, char *argv[]) {
  printf("SYMBOSVM b%s\n",c_build);
  preinit_system();

  if(argc!=2) {
    if(!doload(z80_mem,var_system_bootfile)) return(-1);
  } else {
    if(!doload(z80_mem,argv[1])) return(-1);
  };

  if(!init_system()) {
    printf("Error initializing System\n");
    return(-1);
  };

  init_drives();
  init_video();
  init_audio();
  init_z80();

  run_z80();

  done_audio();
  done_video();
  done_system();
  done_drives();

  return(0);
};
