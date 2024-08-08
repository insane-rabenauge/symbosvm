/*
 * This file is part of the SymbosVM project, which is distributed
 * under the terms of the GNU General Public License; either
 * version 2 of the License, or (at your option) any later version.
 */

// written by insane/Rabenauge^.tSCc.

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "system.h"
#include "drives.h"
#include "simz80.h"
#include "config.h"
#include "platform.h"

drivetype drives[MAX_DRIVES];
int drvstat=0;

int drive_open(int drive,char* file) {
  if (drive>MAX_DRIVES) return 0;
  drives[drive].fp=fopen(file,"r+b");
  if (drives[drive].fp==NULL) return 0;
//  setvbuf(drives[drive].fp, NULL, _IOFBF, 4096);
  drives[drive].ready=1;
  return 1;
};

int drive_close(int drive) {
  if (drive>MAX_DRIVES) return 0;
  if (drives[drive].ready) fclose(drives[drive].fp);
  drives[drive].ready=0;
  return 0;
};

int drive_getinfo(int drive,uint32_t* seccnt) {
  if (drive>MAX_DRIVES) return 0;
  if (seccnt==NULL) return 0;
  if (!drives[drive].ready) return 0;
  OS_FSEEK(drives[drive].fp,0L,SEEK_END);
  uint64_t filesiz=OS_FTELL(drives[drive].fp);
  rewind(drives[drive].fp);
  *seccnt=(filesiz/SECTOR_SIZE);
  return 0;
};

int drive_read(int drive,int seccnt,uint32_t lbasec,uint32_t dmaptr) {
  if (drive>MAX_DRIVES) return 0;
  if (!drives[drive].ready) return 0;
  uint64_t seekptr=(uint64_t)lbasec*SECTOR_SIZE;
  if (OS_FSEEK(drives[drive].fp,seekptr,SEEK_SET)!=0) return 0;
  if ((dmaptr+(seccnt*SECTOR_SIZE))>MEM_SIZE) return 0;
  drvstat=fread(&z80_mem[dmaptr],SECTOR_SIZE,seccnt,drives[drive].fp);
  return 1;
};

int drive_write(int drive,int seccnt,uint32_t lbasec,uint32_t dmaptr) {
  if (drive>MAX_DRIVES) return 0;
  if (!drives[drive].ready) return 0;
  uint64_t seekptr=(uint64_t)lbasec*SECTOR_SIZE;
  if (OS_FSEEK(drives[drive].fp,seekptr,SEEK_SET)!=0) return 0;
  if ((dmaptr+(seccnt*SECTOR_SIZE))>MEM_SIZE) return 0;
  drvstat=fwrite(&z80_mem[dmaptr],SECTOR_SIZE,seccnt,drives[drive].fp);
  return 1;
};

void init_drives() {
  if (strlen(var_system_hdd_0)) if (drive_open(0,var_system_hdd_0)==0) printf("Error opening Drive 0 Image \"%s\"\n",var_system_hdd_0);
  if (strlen(var_system_hdd_1)) if (drive_open(1,var_system_hdd_1)==0) printf("Error opening Drive 1 Image \"%s\"\n",var_system_hdd_1);
  if (strlen(var_system_hdd_2)) if (drive_open(2,var_system_hdd_2)==0) printf("Error opening Drive 2 Image \"%s\"\n",var_system_hdd_2);
  if (strlen(var_system_hdd_3)) if (drive_open(3,var_system_hdd_3)==0) printf("Error opening Drive 3 Image \"%s\"\n",var_system_hdd_3);
  if (strlen(var_system_hdd_4)) if (drive_open(4,var_system_hdd_4)==0) printf("Error opening Drive 4 Image \"%s\"\n",var_system_hdd_4);
  if (strlen(var_system_hdd_5)) if (drive_open(5,var_system_hdd_5)==0) printf("Error opening Drive 5 Image \"%s\"\n",var_system_hdd_5);
  if (strlen(var_system_hdd_6)) if (drive_open(6,var_system_hdd_6)==0) printf("Error opening Drive 6 Image \"%s\"\n",var_system_hdd_6);
  if (strlen(var_system_hdd_7)) if (drive_open(7,var_system_hdd_7)==0) printf("Error opening Drive 7 Image \"%s\"\n",var_system_hdd_7);
};

void done_drives() {
  for (int i=0;i<MAX_DRIVES;i++) drive_close(i);
};

/* vim: set et ts=2 sw=2 :*/
