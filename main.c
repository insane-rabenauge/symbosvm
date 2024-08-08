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
#include "version.h"

int main(int argc, char *argv[]) {
  printf("SYMBOSVM v%i.%i b%s.%s\n",VERSION_MAJOR,VERSION_MINOR,SYMBOSVM_BUILD_D,SYMBOSVM_BUILD_T);
  preinit_system();

  if(argc!=2) {
    if(!z80_loadrom(var_system_bootfile)) return(-1);
  } else {
    if(!z80_loadrom(argv[1])) return(-1);
  };

  if(!init_system()) {
    printf("Error initializing System\n");
    return(-1);
  };

  init_drives();
  init_video();
  init_audio();
  init_z80();

  z80_execute();

  done_z80();
  done_audio();
  done_video();
  done_system();
  done_drives();

  if (sys_quit_type==D_VMSHUTDOWN) {
    system(var_system_shutdown);
  };

  return(0);
};
