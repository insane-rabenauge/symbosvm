#ifndef _DRIVES_H
#define _DRIVES_H

#define MAX_DRIVES 8
#define SECTOR_SIZE 512

typedef struct {
  FILE *fp;
  int ready;
} drivetype;

void init_drives(void);
void done_drives(void);
int drive_open(int drive,char* file);
int drive_close(int drive);
int drive_getinfo(int drive,uint32_t* seccnt);
int drive_read(int drive,int seccnt,uint32_t lbasec,uint32_t dmaptr);
int drive_write(int drive,int seccnt,uint32_t lbasec,uint32_t dmaptr);

#endif
