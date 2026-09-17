/*
 * This file is part of the SymbosVM project, which is distributed
 * under the terms of the GNU General Public License; either
 * version 2 of the License, or (at your option) any later version.
 */

// written by insane/Rabenauge^.tSCc.

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include <ctype.h>
#include <time.h>
#include <errno.h>
#include "hardware.h"
#include "hostfs.h"
#include "config.h"
#include "simz80.h"
#ifdef _WIN32
  #include <direct.h>
  #include <windows.h>
#else
  #include <sys/stat.h>
  #include <sys/types.h>
  #include <sys/statvfs.h>
  #include <utime.h>
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#ifdef _WIN32
#include <windows.h>
#define PATH_SEP '\\'
#else
#define PATH_SEP '/'
#endif

#define MAX_FILES 256 // handles to use, from symbos, max. 8 concurrent
#define DRVMSK 7 // drives 0-7

static char hostname[PATH_MAX];
static char hostcwd[PATH_MAX];

static char *hostpaths[8];

static FILE *hostfiles[MAX_FILES];

uint8_t hostfs_data[16];

size_t strlcpy(char *dst, const char *src, size_t size) {
  size_t src_len = 0;
  const char *s = src;

  while (*s++) src_len++;

  if (size != 0) {
    size_t to_copy = (src_len >= size) ? (size - 1) : src_len;
    for (size_t i = 0; i < to_copy; i++)
      dst[i] = src[i];
    dst[to_copy] = '\0';
  }

  return src_len;
}

static int host_mkdir(const char *path) {
#ifdef _WIN32
  if (_mkdir(path) == 0) return HFSERR_OK;
#else
  if (mkdir(path, 0755) == 0) return HFSERR_OK;
#endif
  if (errno == EEXIST) return HFSERRDBL;
  return HFSERRACS;
}


size_t strlcat(char *dst, const char *src, size_t size) {
  size_t dst_len = 0;
  size_t src_len = 0;
  const char *s;

  s = dst;
  while (dst_len < size && *s) {
    dst_len++;
    s++;
  }

  s = src;
  while (*s++) src_len++;

  if (dst_len == size)
    return size + src_len;

  size_t to_copy = (src_len >= size - dst_len) ? (size - dst_len - 1) : src_len;
  for (size_t i = 0; i < to_copy; i++)
    dst[dst_len + i] = src[i];
  dst[dst_len + to_copy] = '\0';

  return dst_len + src_len;
}

static inline void normalize_separators(char *s) {
#ifdef _WIN32
  for (char *p = s; *p; ++p) if (*p == '/') *p = '\\';
#else
  for (char *p = s; *p; ++p) if (*p == '\\') *p = '/';
#endif
}

static uint16_t timetodostime(time_t t) {
  struct tm lt;
#ifdef _WIN32
  localtime_s(&lt, &t);
#else
  localtime_r(&t, &lt);
#endif

  return ((lt.tm_hour&0x1F)<<11)|((lt.tm_min&0x3F)<<5)|((lt.tm_sec>>1)&0x1F);
}

static uint16_t timetodosdate(time_t t) {
  struct tm lt;
#ifdef _WIN32
  localtime_s(&lt, &t);
#else
  localtime_r(&t, &lt);
#endif

  return (((lt.tm_year-80)&0x7F)<<9)|(((lt.tm_mon+1)&0x0F)<<5)|(lt.tm_mday&0x1F);
}

static time_t dostime_to_unix(uint16_t dostime, uint16_t dosdate) {
  struct tm lt;
  memset(&lt, 0, sizeof(lt));
  lt.tm_hour = (dostime >> 11) & 0x1F;
  lt.tm_min  = (dostime >> 5)  & 0x3F;
  lt.tm_sec  = (dostime & 0x1F) << 1;
  lt.tm_year = ((dosdate >> 9) & 0x7F) + 80;
  lt.tm_mon  = ((dosdate >> 5) & 0x0F) - 1;
  lt.tm_mday = dosdate & 0x1F;
  lt.tm_isdst = -1;
  return mktime(&lt);
}

static int name_match(const char *name, const char *mask) {
  if ((mask[0]==0)||(strcmp(mask,"*.*")==0)) {
    return 1;
  };
  while (*mask) {
    if (*mask == '*') {
      mask++;
      if (!*mask) return 1;
      while (*name) {
        if (name_match(name, mask)) return 1;
        name++;
      }
      return 0;
    } else if (*mask == '?') {
      if (!*name) return 0;
      name++;
      mask++;
    } else {
      if (*mask != *name) return 0;
      name++;
      mask++;
    }
  }
  return *name == '\0';
}

static void split_dir_mask(char *dirpath_mask, char *dirpath, size_t dirpath_sz,
                            char *mask, size_t mask_sz) {
  normalize_separators(dirpath_mask);
  const char *p = strrchr(dirpath_mask, PATH_SEP);
  if (p) {
    size_t n = p - dirpath_mask;
    if (n >= dirpath_sz) n = dirpath_sz - 1;
    strncpy(dirpath, dirpath_mask, n);
    dirpath[n] = 0;
    strlcpy(mask, p + 1, mask_sz);
  } else {
    strlcpy(dirpath, ".", dirpath_sz);
    strlcpy(mask, dirpath_mask, mask_sz);
  }
}

// Case-insensitive path resolver for Linux
#ifndef _WIN32
static int str_casecmp(const char *a, const char *b) {
  for (; *a && *b; a++, b++) {
    int ca = tolower((unsigned char)*a);
    int cb = tolower((unsigned char)*b);
    if (ca != cb) return ca - cb;
  }
  return *a - *b;
}

static int find_case_insensitive_path(const char *input, char *resolved, size_t resolved_size) {
  char *out = resolved;
  const char *p = input;

  if (*p == '/') {
    *out++ = '/';
    *out = '\0';
    p++;
  } else {
    strlcpy(resolved, hostcwd, resolved_size);
    out = resolved + strlen(resolved);
    *out++ = '/';
    *out = '\0';
  }

  while (*p) {
    char part[256];
    char *q = part;

    while (*p && *p != '/') *q++ = *p++;
    *q = '\0';
    if (*p == '/') p++;

    DIR *dir = opendir(resolved);
    if (!dir) return 0;

    struct dirent *de;
    int found = 0;
    while ((de = readdir(dir)) != NULL) {
      if (str_casecmp(de->d_name, part) == 0) {
        size_t len = strlen(de->d_name);
        if ((out - resolved) + len + 1 >= resolved_size) {
          closedir(dir);
          return 0;
        }
        strcpy(out, de->d_name);
        out += len;
        *out++ = '/';
        *out = '\0';
        found = 1;
        break;
      }
    }
    closedir(dir);
    if (!found) {
      strcpy(out,part);
      out += strlen(part);
      *out++ = '/';
      *out = '\0';
      found = 1;
      return 1;
    }
  }

  if (out - resolved > 1 && *(out - 1) == '/') *(out - 1) = '\0';
  return 1;
}
#endif

// Normalize path (remove ., ..)
static void normalize_path(char *path) {
  char *parts[256];
  int count = 0;
  char tmp[4096];
  char *p = tmp;
  char *workpath = path;

#ifdef _WIN32
  char drive[3] = {0};
  if (isalpha((unsigned char)path[0]) && path[1] == ':') {
    drive[0] = path[0];
    drive[1] = ':';
    workpath += 2;
  }
#endif

  char *token = strtok(workpath, "/\\");
  while (token) {
    if (strcmp(token, ".") == 0) {
      // skip
    } else if (strcmp(token, "..") == 0) {
      if (count > 0) count--;
    } else {
      parts[count++] = token;
    }
    token = strtok(NULL, "/\\");
  }

#ifdef _WIN32
  if (drive[0]) {
    p += sprintf(p, "%s%c", drive, PATH_SEP);
  } else {
    *p++ = PATH_SEP;
  }
#else
  *p++ = PATH_SEP;
#endif

  for (int i = 0; i < count; i++) {
    size_t len = strlen(parts[i]);
    memcpy(p, parts[i], len);
    p += len;
    if (i != count - 1) *p++ = PATH_SEP;
  }
  *p = '\0';
  strcpy(path, tmp);
}

int hostfs_resolve(char *root, char *guest, char *out, size_t outsz) {
  char abs_root[4096+4];
  char combined[4096+4];
  if (var_debug_hostfs>=3) printf("HOSTFS:RESOLVE:#%s#:#%s#\n",root,guest);

  // Resolve root to absolute
#ifdef _WIN32
  if (!(isalpha((unsigned char)root[0]) && root[1] == ':')) {
    _fullpath(abs_root, root, sizeof(abs_root));
  } else {
    strlcpy(abs_root, root, sizeof(abs_root));
  }
#else
  if (root[0] != '/') {
    strlcpy(abs_root,hostcwd,sizeof(abs_root));
    strncat(abs_root, "/", sizeof(abs_root) - strlen(abs_root) - 1);
    strncat(abs_root, root, sizeof(abs_root) - strlen(abs_root) - 1);
  } else {
    strlcpy(abs_root, root, sizeof(abs_root));
  }
#endif
  normalize_path(abs_root);

  // Skip guest drive letter
  if (isalpha((unsigned char)guest[0]) && guest[1] == ':') {
    guest += 2;
    if (*guest == '/' || *guest == '\\') guest++;
  }

  // Combine
  snprintf(combined, sizeof(combined), "%s%c%s", abs_root, PATH_SEP, guest);
  normalize_path(combined);

  // Prefix check
#ifdef _WIN32
  if (_strnicmp(combined, abs_root, strlen(abs_root)) != 0) return 0;
#else
  if (strncmp(combined, abs_root, strlen(abs_root)) != 0) return 0;
#endif

#ifndef _WIN32
  // Fix casing from filesystem
  if (!find_case_insensitive_path(combined, out, outsz)) return 0;
#else
  strncpy(out, combined, outsz);
#endif

  return 1;
}

static void hostfs_setresult(uint8_t res) {
  hostfs_data[0]=res;
};

static void hostfs_file_new(void) {
/*
File_New:
  Creates a new file and opens it for read/write access. If the file was already existing,
  it will be emptied first. The operation will be aborted, if the existing file is read only or
  an sub directory.

I:H-CMD: ID for File_New (017)
I:DAT#1: File-ID to use
I:DAT#2: HostFS Drive to use

I:DAT#3: Attributes (ignored for now)
I:DAT#8: File path and name address low
I:DAT#9: File path and name address high
I:DAT#B: File path and name address upper

O:H-CMD: Status
*/
  uint32_t z80fileptr=(hostfs_data[0xb]<<16)|(hostfs_data[0x9]<<8)|(hostfs_data[0x8]);
  uint8_t fileid=hostfs_data[0x1];
  uint8_t drive=hostfs_data[0x2];
  char *fileptr=(char*)&z80_mem[z80fileptr];
  if (var_debug_hostfs>=1) printf("HOSTFS:%s:DRIVE %i:FILE ID %02X:PTR %06X:#%s#\n",__func__,drive,fileid,z80fileptr,fileptr);
  if (hostfiles[fileid]!=NULL) {
    fclose(hostfiles[fileid]);
    hostfiles[fileid]=NULL;
  };
  if (hostfs_resolve(hostpaths[drive&DRVMSK],fileptr,hostname,sizeof(hostname))) {
    if (var_debug_hostfs>=1) printf("HOSTFS:%s:opening file #%s#\n",__func__,hostname);
    hostfiles[fileid]=fopen(hostname,"w+b"); // default symbos mode is rw
    if (hostfiles[fileid]==NULL) {
      hostfs_setresult(HFSERRNAM);
    } else {
      hostfs_setresult(HFSERR_OK);
    };
  } else {
    hostfs_setresult(HFSERRACS);
  };
};

static void hostfs_file_open(void) {
/*
File_Open:
  Opens an existing file for read/write access.

I:H-CMD: ID for File_Open (018)
I:DAT#1: File-ID to use
I:DAT#2: HostFS Drive to use

I:DAT#8: File path and name address low
I:DAT#9: File path and name address high
I:DAT#B: File path and name address upper

O:H-CMD: Status
*/
  uint32_t z80fileptr=(hostfs_data[0xb]<<16)|(hostfs_data[0x9]<<8)|(hostfs_data[0x8]);
  uint8_t fileid=hostfs_data[0x1];
  uint8_t drive=hostfs_data[0x2];
  char *fileptr=(char*)&z80_mem[z80fileptr];
  if (var_debug_hostfs>=1) printf("HOSTFS:%s:DRIVE %i:FILE ID %02X:PTR %06X:#%s#\n",__func__,drive,fileid,z80fileptr,fileptr);
  if (hostfiles[fileid]!=NULL) {
    fclose(hostfiles[fileid]);
    hostfiles[fileid]=NULL;
  };
  if (hostfs_resolve(hostpaths[drive&DRVMSK],fileptr,hostname,sizeof(hostname))) {
    if (var_debug_hostfs>=1) printf("HOSTFS:%s:opening file #%s#\n",__func__,hostname);
    hostfiles[fileid]=fopen(hostname,"r+b"); // default symbos mode is rw
    if (hostfiles[fileid]==NULL) {
      hostfs_setresult(HFSERRNAM);
    } else {
      hostfs_setresult(HFSERR_OK);
    };
  } else {
    hostfs_setresult(HFSERRACS);
  };
};

static void hostfs_file_close(void) {
/*
File_Close:
  Closes an opened file.

I:H-CMD: ID for File_Close (019)
I:DAT#1: File-ID to use

O:H-CMD: Status
*/
  uint8_t fileid=hostfs_data[0x1];
  if (var_debug_hostfs>=1) printf("HOSTFS:%s:FILE ID %02X\n",__func__,fileid);
  if (hostfiles[fileid]!=NULL) {
    fclose(hostfiles[fileid]);
    hostfiles[fileid]=NULL;
  };
  hostfs_setresult(HFSERR_OK);
};

static void hostfs_file_input(void) {
/*
File_Input:
  Reads a specified amount of bytes out of an opened file. After this operation the file pointer will be
  moved behind the last read byte. This means, that it is possible to read several blocks with different
  sizes out of an opened file. It doesn't matter, if you already did write operations, too.
  If you try to read more bytes than available, the zero flag will be reset.

I:H-CMD: ID for File_Input (020)
I:DAT#1: File-ID to use

I:DAT#4: bytes to read low
I:DAT#5: bytes to read high
I:DAT#6: destination address upper
I:DAT#8: destination address low
I:DAT#9: destination address high

O:H-CMD: Status
O:DAT#8: number of read bytes low
O:DAT#9: number of read bytes high
*/
  uint8_t fileid=hostfs_data[0x1];
  uint16_t btr=(hostfs_data[0x5]<<8)|hostfs_data[0x4];
  uint32_t z80memptr=(hostfs_data[0x6]<<16)|(hostfs_data[0x9]<<8)|(hostfs_data[0x8]);
  char *memptr=(char*)&z80_mem[z80memptr];
  if (var_debug_hostfs>=1) printf("HOSTFS:%s:FILE ID %02X:PTR %06X:LEN %04X:FPOS %08X\n",__func__,fileid,z80memptr,btr,(uint32_t)ftell(hostfiles[fileid]));
  if (hostfiles[fileid]==NULL) {
    hostfs_setresult(HFSERRHND);
    return;
  };
  int btrres=fread(memptr,1,btr,hostfiles[fileid]);
  if (var_debug_hostfs>=1) printf("HOSTFS:%s:%04X BYTES:FPOS %08X\n",__func__,btrres,(uint32_t)ftell(hostfiles[fileid]));
  hostfs_data[0x8]=btrres;
  hostfs_data[0x9]=btrres>>8;
  hostfs_setresult(HFSERR_OK);
};

static void hostfs_file_output(void) {
/*
File_Output:
  Writes a specified amount of bytes into an opened file. After this operation the file pointer will be
  moved behind the last written byte. If the file pointer has been somewhere in the middle of the file
  before this operation, the data at this place will be overwritten. If you have been at the end of the
  file, its length will be extended.

I:H-CMD: ID for File_Output (021)
I:DAT#1: File-ID to use

I:DAT#4: bytes to write low
I:DAT#5: bytes to write high
I:DAT#6: source address upper
I:DAT#8: source address low
I:DAT#9: source address high

O:H-CMD: Status
O:DAT#8: number of written bytes low
O:DAT#9: number of written bytes high
*/
  uint8_t fileid=hostfs_data[0x1];
  uint16_t btw=(hostfs_data[0x5]<<8)|hostfs_data[0x4];
  uint32_t z80memptr=(hostfs_data[0x6]<<16)|(hostfs_data[0x9]<<8)|(hostfs_data[0x8]);
  char *memptr=(char*)&z80_mem[z80memptr];
  if (var_debug_hostfs>=1) printf("HOSTFS:%s:FILE ID %02X:PTR %06X:LEN %04X\n",__func__,fileid,z80memptr,btw);
  if (hostfiles[fileid]==NULL) {
    hostfs_setresult(HFSERRHND);
    return;
  };
  int btwres=fwrite(memptr,1,btw,hostfiles[fileid]);
  if (var_debug_hostfs>=1) printf("HOSTFS:%s:%04X BYTES\n",__func__,btwres);
  hostfs_data[0x8]=btwres;
  hostfs_data[0x9]=btwres>>8;
  hostfs_setresult(HFSERR_OK);
};

static void hostfs_file_pointer(void) {
/*
File_Pointer:
  fseek/ftell

I:H-CMD: ID for File_Pointer (022)
I:DAT#1: File-ID to use

I:DAT#4: reference point
I:DAT#A: file pointer b0-b7
I:DAT#B: file pointer b8-b15
I:DAT#C: file pointer b16-b23
I:DAT#D: file pointer b24-b31

O:H-CMD: Status
O:DAT#A: file pointer b0-b7
O:DAT#B: file pointer b8-b15
O:DAT#C: file pointer b16-b23
O:DAT#D: file pointer b24-b31
*/
  uint8_t fileid=hostfs_data[0x1];
  uint8_t svmwhence=hostfs_data[0x4];
  long offset=(hostfs_data[0xD]<<24)|(hostfs_data[0xC]<<16)|(hostfs_data[0xB]<<8)|(hostfs_data[0xA]);
  if (var_debug_hostfs>=1) printf("HOSTFS:%s:FILE ID %02X:RP %X:PTR %li\n",__func__,fileid,svmwhence,offset);
  if (hostfiles[fileid]==NULL) {
    hostfs_setresult(HFSERRHND);
    return;
  };
  int whence=0;
  switch(svmwhence) {
    case 0:
      whence=SEEK_SET;
      break;
    case 1:
      whence=SEEK_CUR;
      break;
    case 2:
      whence=SEEK_END;
      break;
  };
  fseek(hostfiles[fileid],offset,whence);
  long fpos=ftell(hostfiles[fileid]);
  if (var_debug_hostfs>=1) printf("HOSTFS:%s:%li POSITION\n",__func__,fpos);
  hostfs_data[0xA]=fpos;
  hostfs_data[0xB]=fpos>>8;
  hostfs_data[0xC]=fpos>>16;
  hostfs_data[0xD]=fpos>>24;
  hostfs_setresult(HFSERR_OK);
};

static void hostfs_directory_property_set(void) {
/*
Directory_Property_Set
  Changes a property of a file or a directory. You can set the attribute,
  the "created" time and the "modified" time.

I:H-CMD: ID for Directory_Property_Set (034)
I:DAT#2: HostFS Drive to use

I:DAT#3: Property Type (0=Attribute, 1=Modified, 2=Created)
I:DAT#8: File path and name address low
I:DAT#9: File path and name address high
I:DAT#B: File path and name address upper

I:DAT#4: time code lo (C-reg) or attribute
I:DAT#5: time code hi (B-reg)
I:DAT#6: date code lo (E-reg)
I:DAT#7: date code hi (D-reg)

O:H-CMD: Status
*/
  uint8_t drive=hostfs_data[0x2];
  uint8_t proptype=hostfs_data[0x3];
  uint32_t z80pathptr=(hostfs_data[0xb]<<16)|(hostfs_data[0x9]<<8)|(hostfs_data[0x8]);
  char *pathptr=(char*)&z80_mem[z80pathptr];
  if (var_debug_hostfs>=1) printf("HOSTFS:%s:DRIVE %i:TYPE %02X:#%s#\n",__func__,drive,proptype,pathptr);

  if (!hostfs_resolve(hostpaths[drive&DRVMSK],pathptr,hostname,sizeof(hostname))) {
    hostfs_setresult(HFSERRACS);
    return;
  };

  if (proptype==HFSPROP_ATTRIBUTE) {
    uint8_t attr=hostfs_data[0x4];
#ifdef _WIN32
    DWORD w=GetFileAttributesA(hostname);
    if (w==INVALID_FILE_ATTRIBUTES) { hostfs_setresult(HFSERRXFI); return; }
    DWORD nw=w & FILE_ATTRIBUTE_DIRECTORY; // not settable, preserve it
    if (attr&ATTR_READONLY) nw|=FILE_ATTRIBUTE_READONLY;
    if (attr&ATTR_HIDDEN)   nw|=FILE_ATTRIBUTE_HIDDEN;
    if (attr&ATTR_SYSTEM)   nw|=FILE_ATTRIBUTE_SYSTEM;
    if (attr&ATTR_ARCHIVE)  nw|=FILE_ATTRIBUTE_ARCHIVE;
    if (nw==0) nw=FILE_ATTRIBUTE_NORMAL;
    hostfs_setresult(SetFileAttributesA(hostname,nw) ? HFSERR_OK : HFSERRACS);
#else
    struct stat st;
    if (stat(hostname,&st)!=0) { hostfs_setresult(HFSERRXFI); return; }
    mode_t mode=st.st_mode;
    if (attr&ATTR_READONLY) mode &= ~(mode_t)(S_IWUSR|S_IWGRP|S_IWOTH);
    else mode |= S_IWUSR;
    hostfs_setresult(chmod(hostname,mode)==0 ? HFSERR_OK : HFSERRACS);
#endif
    return;
  }

  if (proptype==HFSPROP_CREATED || proptype==HFSPROP_MODIFIED) {
    uint16_t packedtime=(hostfs_data[0x5]<<8)|hostfs_data[0x4];
    uint16_t packeddate=(hostfs_data[0x7]<<8)|hostfs_data[0x6];
    time_t t=dostime_to_unix(packedtime,packeddate);
#ifdef _WIN32
    HANDLE h=CreateFileA(hostname,GENERIC_WRITE,FILE_SHARE_READ|FILE_SHARE_WRITE,
                          NULL,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,NULL);
    if (h==INVALID_HANDLE_VALUE) { hostfs_setresult(HFSERRXFI); return; }
    ULARGE_INTEGER ull;
    ull.QuadPart=(ULONGLONG)t*10000000ULL+116444736000000000ULL;
    FILETIME ft; ft.dwLowDateTime=ull.LowPart; ft.dwHighDateTime=ull.HighPart;
    BOOL ok=(proptype==HFSPROP_CREATED) ? SetFileTime(h,&ft,NULL,NULL)
                                         : SetFileTime(h,NULL,NULL,&ft);
    CloseHandle(h);
    hostfs_setresult(ok ? HFSERR_OK : HFSERRGEN);
#else
    if (proptype==HFSPROP_CREATED) { hostfs_setresult(HFSERRSUP); return; }
    struct stat st;
    if (stat(hostname,&st)!=0) { hostfs_setresult(HFSERRXFI); return; }
    struct utimbuf times;
    times.actime=st.st_atime; // leave access time untouched
    times.modtime=t;
    hostfs_setresult(utime(hostname,&times)==0 ? HFSERR_OK : HFSERRACS);
#endif
    return;
  }

  hostfs_setresult(HFSERRATR); // unknown property type
};

static void hostfs_directory_property_get(void) {
/*
Directory_Property_Get
  Reads a property of a file or a directory.

I:H-CMD: ID for Directory_Property_Get (035)
I:DAT#2: HostFS Drive to use

I:DAT#3: Property Type (0=Attribute, 1=Modified, 2=Created)
I:DAT#8: File path and name address low
I:DAT#9: File path and name address high
I:DAT#B: File path and name address upper

O:H-CMD: Status
O:DAT#4: time code lo (C-reg) or attribute
O:DAT#5: time code hi (B-reg)
O:DAT#6: date code lo (E-reg)
O:DAT#7: date code hi (D-reg)
*/
  uint8_t drive=hostfs_data[0x2];
  uint8_t proptype=hostfs_data[0x3];
  uint32_t z80pathptr=(hostfs_data[0xb]<<16)|(hostfs_data[0x9]<<8)|(hostfs_data[0x8]);
  char *pathptr=(char*)&z80_mem[z80pathptr];
  if (var_debug_hostfs>=1) printf("HOSTFS:%s:DRIVE %i:TYPE %02X:#%s#\n",__func__,drive,proptype,pathptr);

  if (!hostfs_resolve(hostpaths[drive&DRVMSK],pathptr,hostname,sizeof(hostname))) {
    hostfs_setresult(HFSERRACS);
    return;
  };

  struct stat st;
  if (stat(hostname,&st)!=0) { hostfs_setresult(HFSERRXFI); return; }

  if (proptype==HFSPROP_ATTRIBUTE) {
    uint8_t attr=0;
#ifdef _WIN32
    DWORD w=GetFileAttributesA(hostname);
    if (w==INVALID_FILE_ATTRIBUTES) { hostfs_setresult(HFSERRXFI); return; }
    if (w&FILE_ATTRIBUTE_READONLY)  attr|=ATTR_READONLY;
    if (w&FILE_ATTRIBUTE_HIDDEN)    attr|=ATTR_HIDDEN;
    if (w&FILE_ATTRIBUTE_SYSTEM)    attr|=ATTR_SYSTEM;
    if (w&FILE_ATTRIBUTE_DIRECTORY) attr|=ATTR_DIRECTORY;
    if (w&FILE_ATTRIBUTE_ARCHIVE)   attr|=ATTR_ARCHIVE;
#else
    if (!(st.st_mode & S_IWUSR)) attr|=ATTR_READONLY;
    if (S_ISDIR(st.st_mode)) attr|=ATTR_DIRECTORY;
#endif
    hostfs_data[0x4]=attr;
    hostfs_data[0x5]=0;
    hostfs_data[0x6]=0;
    hostfs_data[0x7]=0;
    hostfs_setresult(HFSERR_OK);
    return;
  }

  if (proptype==HFSPROP_CREATED || proptype==HFSPROP_MODIFIED) {
    time_t t;
#ifdef _WIN32
    HANDLE h=CreateFileA(hostname,GENERIC_READ,FILE_SHARE_READ|FILE_SHARE_WRITE,
                          NULL,OPEN_EXISTING,FILE_FLAG_BACKUP_SEMANTICS,NULL);
    if (h==INVALID_HANDLE_VALUE) { hostfs_setresult(HFSERRXFI); return; }
    FILETIME created,modified;
    BOOL ok=GetFileTime(h,&created,NULL,&modified);
    CloseHandle(h);
    if (!ok) { hostfs_setresult(HFSERRGEN); return; }
    FILETIME *ft=(proptype==HFSPROP_CREATED)?&created:&modified;
    ULARGE_INTEGER ull; ull.LowPart=ft->dwLowDateTime; ull.HighPart=ft->dwHighDateTime;
    t=(time_t)((ull.QuadPart-116444736000000000ULL)/10000000ULL);
#else
    if (proptype==HFSPROP_CREATED) { hostfs_setresult(HFSERRSUP); return; }
    t = st.st_mtime;
#endif
    uint16_t packedtime=timetodostime(t);
    uint16_t packeddate=timetodosdate(t);
    hostfs_data[0x4]=packedtime;
    hostfs_data[0x5]=packedtime>>8;
    hostfs_data[0x6]=packeddate;
    hostfs_data[0x7]=packeddate>>8;
    hostfs_setresult(HFSERR_OK);
    return;
  }

  hostfs_setresult(HFSERRATR); // unknown property type
};

static void hostfs_directory_rename(void) {
/*
Directory_Rename
  Renames a file or a directory. The new file name must not include a path.
  The function will fail, if a file or directory with the new name already exists.

I:H-CMD: ID for Directory_Rename (036)
I:DAT#2: HostFS Drive to use

I:DAT#8: File path and old name address low
I:DAT#9: File path and old name address high
I:DAT#B: address upper for file path and old name and new name
I:DAT#6: New name address low
I:DAT#7: New name address high

O:H-CMD: Status
*/
  uint8_t drive=hostfs_data[0x2];
  uint32_t z80oldptr=(hostfs_data[0xb]<<16)|(hostfs_data[0x9]<<8)|(hostfs_data[0x8]);
  uint32_t z80newptr=(hostfs_data[0xb]<<16)|(hostfs_data[0x7]<<8)|(hostfs_data[0x6]);
  char *oldptr=(char*)&z80_mem[z80oldptr];
  char *newname=(char*)&z80_mem[z80newptr];

  if (var_debug_hostfs>=1) printf("HOSTFS:%s:DRIVE %i:OLD #%s#:NEW #%s#\n",__func__,drive,oldptr,newname);

  if (strchr(newname,'/') || strchr(newname,'\\')) {
    hostfs_setresult(HFSERRNNA);
    return;
  }

  char oldresolved[PATH_MAX];
  if (!hostfs_resolve(hostpaths[drive&DRVMSK],oldptr,oldresolved,sizeof(oldresolved))) {
    hostfs_setresult(HFSERRACS);
    return;
  };

  char guestold[PATH_MAX];
  strlcpy(guestold,oldptr,sizeof(guestold));
  normalize_separators(guestold);
  char guestnew[PATH_MAX];
  char *slash=strrchr(guestold,PATH_SEP);
  if (slash) {
    size_t dirlen=slash-guestold+1;
    if (dirlen>=sizeof(guestnew)) dirlen=sizeof(guestnew)-1;
    memcpy(guestnew,guestold,dirlen);
    guestnew[dirlen]=0;
    strlcat(guestnew,newname,sizeof(guestnew));
  } else {
    strlcpy(guestnew,newname,sizeof(guestnew));
  }

  char newresolved[PATH_MAX];
  if (!hostfs_resolve(hostpaths[drive&DRVMSK],guestnew,newresolved,sizeof(newresolved))) {
    hostfs_setresult(HFSERRACS);
    return;
  };

  struct stat st;
  if (stat(newresolved,&st)==0) {
    hostfs_setresult(HFSERRDBL); // destination already exists
    return;
  }

  if (rename(oldresolved,newresolved)==0) {
    hostfs_setresult(HFSERR_OK);
  } else {
#ifdef _WIN32
    hostfs_setresult(HFSERRACS);
#else
    hostfs_setresult((errno==ENOENT) ? HFSERRXDI : HFSERRACS);
#endif
  }
};

static void hostfs_directory_new(void) {
/*
Directory_New
  Creates a new directory. The function will fail,
  if a file or directory with the same name already exists.

I:H-CMD: ID for Directory_New (037)
I:DAT#2: HostFS Drive to use

I:DAT#8: File path and name address low
I:DAT#9: File path and name address high
I:DAT#B: File path and name address upper
*/
  uint32_t z80fileptr=(hostfs_data[0xb]<<16)|(hostfs_data[0x9]<<8)|(hostfs_data[0x8]);
  uint8_t drive=hostfs_data[0x2];
  char *dirptr=(char*)&z80_mem[z80fileptr];
  if (hostfs_resolve(hostpaths[drive&DRVMSK],dirptr,hostname,sizeof(hostname))) {
    if (var_debug_hostfs>=1) printf("HOSTFS:%s:directory new #%s#\n",__func__,hostname);
    hostfs_setresult(host_mkdir(hostname));
  } else {
    hostfs_setresult(HFSERRACS);
  };
};

static void hostfs_directory_input(void) {
/*
Directory_Input
  Reads the content of a directory. You can specify a name filter by adding a file mask to the path
  (* and ? are allowed) and an attribute filter.  The system skips the specified amount of entries first
  and then loads as many entries as exist or as there is place in the destination buffer.
  Please note, that the entries will not be sorted. Depending on its name every entry in the destination
  buffer can have a different length and is terminated with 0 behind the file name. The next entry is
  following directly after the 0-terminator.

I:H-CMD: ID for Directory_Input (038)
I:DAT#2: HostFS Drive to use

I:DAT#6: Destination buffer address low
I:DAT#7: Destination buffer address high
I:DAT#3: Destination buffer address upper
I:DAT#4: Destination buffer length low
I:DAT#5: Destination buffer length high
I:DAT#8: Directory path address low
I:DAT#9: Directory path address high
I:DAT#B: Directory path address upper
I:DAT#A: Attribute Filter
I:DAT#C: Number of entires, which should be skipped low
I:DAT#D: Number of entires, which should be skipped high

O:H-CMD: Status
O:DAT#4: Remaining unused space in the destination buffer low
O:DAT#5: Remaining unused space in the destination buffer high
O:DAT#8: Number of read entries low
O:DAT#9: Number of read entries high
*/
  uint8_t drive=hostfs_data[0x2];
  uint32_t z80destbufptr=(hostfs_data[0x3]<<16)|(hostfs_data[0x7]<<8)|(hostfs_data[0x6]);
  uint32_t z80pathptr=(hostfs_data[0xb]<<16)|(hostfs_data[0x9]<<8)|(hostfs_data[0x8]);
  int dest_len=(hostfs_data[0x5]<<8)|(hostfs_data[0x4]);
  int attr_filter=hostfs_data[0xA];
  int skip_entries=(hostfs_data[0xD]<<8)|(hostfs_data[0xC]);
  char *dirpath_mask=(char*)&z80_mem[z80pathptr];
  char *dest=(char*)&z80_mem[z80destbufptr];
  if (var_debug_hostfs>=1) printf("HOSTFS:%s:DRIVE %i(%s):DESTBUF %06X:DESTLEN %04X:PATHBUF %06X:ATTR %02X:SKIP %04X\n",__func__,drive,hostpaths[drive&DRVMSK],z80destbufptr,dest_len,z80pathptr,attr_filter,skip_entries);

  size_t written = 0;
  size_t read_entries = 0;

  // split directory and mask
  char dirpath[PATH_MAX];
  char mask[256];
  split_dir_mask(dirpath_mask, dirpath, sizeof(dirpath), mask, sizeof(mask));

  if (var_debug_hostfs>=3) printf("HOSTFS:%s:#%s# to DIR #%s# and MASK #%s#\n",__func__,dirpath_mask,dirpath,mask);
  if (hostfs_resolve(hostpaths[drive&DRVMSK],dirpath,hostname,sizeof(hostname))==0) {
    if (var_debug_hostfs>=3) printf("HOSTFS:%s:can't resolve %s/%s\n",__func__,hostpaths[drive&DRVMSK],dirpath);
    hostfs_setresult(HFSERRACS);
    return;
  };
  if (var_debug_hostfs>=3) printf("HOSTFS:%s:resolved to #%s#\n",__func__,hostname);
  DIR *d = opendir(hostname);
  if (!d) {
    hostfs_data[0x4]=dest_len;
    hostfs_data[0x5]=dest_len>>8;
    hostfs_data[0x8]=0;
    hostfs_data[0x9]=0;
    hostfs_setresult(HFSERRNAM);
    if (var_debug_hostfs>=3) printf("HOSTFS:%s:opendir error\n",__func__);
    return;
  }

  struct dirent *de;
  while ((de = readdir(d))) {
    const char *name = de->d_name;
    if (strcmp(name,".")==0) continue;
    if (var_debug_hostfs>=3) printf("HOSTFS:%s:found #%s#\n",__func__,name);

    struct stat st;
    char full[1024];
    char statname[PATH_MAX];
    snprintf(full, sizeof(full), "%s/%s", dirpath, name);
    if (hostfs_resolve(hostpaths[drive&DRVMSK],full,statname,PATH_MAX)==0) {
      if (var_debug_hostfs>=3) printf("HOSTFS:%s:can't resolve %s\n",__func__,full);
      continue;
    };
    if (var_debug_hostfs>=3) printf("HOSTFS:%s:stat #%s#\n",__func__,statname);
    if (stat(statname, &st) != 0) continue;

    uint8_t attr = 0;
    if (!(st.st_mode & S_IWUSR)) attr |= ATTR_READONLY;
    //if (name[0] == '.') attr |= ATTR_HIDDEN;
    if (S_ISDIR(st.st_mode)) attr |= ATTR_DIRECTORY;
    //else attr |= ATTR_ARCHIVE;
    if (var_debug_hostfs>=3) printf("HOSTFS:%s:matching with #%s#\n",__func__,mask);
    if ((!(S_ISDIR(st.st_mode)))&&(!name_match(name, mask))) continue;
    if (var_debug_hostfs>=3) printf("HOSTFS:%s:matched with #%s#\n",__func__,mask);

    if (var_debug_hostfs>=3) printf("HOSTFS:%s:checking attribute %08X %08X\n",__func__,attr,attr_filter);
    if ((attr & attr_filter) != 0) continue;
    if (skip_entries > 0) { skip_entries--; continue; }

    uint32_t fsize = S_ISDIR(st.st_mode) ? 0 : (uint32_t)st.st_size;

    uint16_t  d=timetodosdate(st.st_mtime);
    uint16_t tm=timetodostime(st.st_mtime);

    size_t entry_len = 9 + strlen(name) + 1;
    if (written + entry_len > dest_len) break;

    memcpy(dest + written, &fsize, 4);
    memcpy(dest + written + 4, &tm, 2);
    memcpy(dest + written + 6, &d, 2);
    dest[written + 8] = attr;
    strcpy((char *)(dest + written + 9), name);
    written += entry_len;
    read_entries++;
    if (var_debug_hostfs>=3) printf("HOSTFS:%s:entry added\n",__func__);
  }
  closedir(d);

  int bufrem=dest_len-written;
  hostfs_data[0x4]=bufrem;
  hostfs_data[0x5]=bufrem>>8;
  hostfs_data[0x8]=read_entries;
  hostfs_data[0x9]=read_entries>>8;
  hostfs_setresult(HFSERR_OK);
  if (var_debug_hostfs>=1) printf("HOSTFS:%s:DESTREM %04X:ENTRIES %04X\n",__func__,bufrem,(int)read_entries);
};

static void hostfs_directory_deletefile(void) {
/*
Directory_DeleteFile
  Deletes one or more files. You can delete multiple files by using a file mask (* and ? are allowed).
  Files, which are read only, can't be deleted.
  This function also can't be used for deleting directories.

I:H-CMD: ID for Directory_DeleteFile (039)
I:DAT#2: HostFS Drive to use

I:DAT#8: File path/mask address low
I:DAT#9: File path/mask address high
I:DAT#B: File path/mask address upper

O:H-CMD: Status
*/
  uint8_t drive=hostfs_data[0x2];
  uint32_t z80pathptr=(hostfs_data[0xb]<<16)|(hostfs_data[0x9]<<8)|(hostfs_data[0x8]);
  char *pathmask=(char*)&z80_mem[z80pathptr];
  if (var_debug_hostfs>=1) printf("HOSTFS:%s:DRIVE %i:#%s#\n",__func__,drive,pathmask);

  char dirpath[PATH_MAX];
  char mask[256];
  split_dir_mask(pathmask, dirpath, sizeof(dirpath), mask, sizeof(mask));

  if (!hostfs_resolve(hostpaths[drive&DRVMSK],dirpath,hostname,sizeof(hostname))) {
    hostfs_setresult(HFSERRACS);
    return;
  };
  DIR *d = opendir(hostname);
  if (!d) {
    hostfs_setresult(HFSERRXDI);
    return;
  }

  int matched=0, deleted=0, readonly_skipped=0;
  struct dirent *de;
  while ((de = readdir(d))) {
    const char *name = de->d_name;
    if (strcmp(name,".")==0 || strcmp(name,"..")==0) continue;

    char full[1024];
    char statname[PATH_MAX];
    snprintf(full, sizeof(full), "%s/%s", dirpath, name);
    if (!hostfs_resolve(hostpaths[drive&DRVMSK],full,statname,sizeof(statname))) continue;

    struct stat st;
    if (stat(statname,&st)!=0) continue;
    if (S_ISDIR(st.st_mode)) continue; // this function only deletes files
    if (!name_match(name, mask)) continue;

    matched++;

#ifndef _WIN32
    if (!(st.st_mode & S_IWUSR)) { readonly_skipped++; continue; }
#else
    DWORD attrs=GetFileAttributesA(statname);
    if (attrs!=INVALID_FILE_ATTRIBUTES && (attrs & FILE_ATTRIBUTE_READONLY)) {
      readonly_skipped++;
      continue;
    }
#endif

    if (remove(statname)==0) deleted++;
  }
  closedir(d);

  if (matched>0 && deleted==0 && readonly_skipped==matched) {
    hostfs_setresult(HFSERRRDO);
    return;
  }

  hostfs_data[0x3] = (uint8_t)(deleted != 0); // nonzero -> SVM's ZF=0 ("something was deleted")
  hostfs_data[0x4] = (uint8_t)deleted;
  hostfs_data[0x5] = (uint8_t)(deleted>>8);
  hostfs_setresult(HFSERR_OK);
};

static void hostfs_directory_deletedirectory(void) {
/*
Directory_DeleteDirectory
  Deletes a sub directory. The sub directory has to be empty and not read only, otherwise the operation
  will be aborted. You can specify the directory with or without "/" at the end of the full path.

I:H-CMD: ID for Directory_DeleteDirectory (040)
I:DAT#2: HostFS Drive to use

I:DAT#8: Directory path address low
I:DAT#9: Directory path address high
I:DAT#B: Directory path address upper
*/
  uint8_t drive=hostfs_data[0x2];
  uint32_t z80pathptr=(hostfs_data[0xb]<<16)|(hostfs_data[0x9]<<8)|(hostfs_data[0x8]);
  char *dirptr=(char*)&z80_mem[z80pathptr];
  if (var_debug_hostfs>=1) printf("HOSTFS:%s:DRIVE %i:#%s#\n",__func__,drive,dirptr);

  char trimmed[PATH_MAX];
  strlcpy(trimmed, dirptr, sizeof(trimmed));
  normalize_separators(trimmed);
  size_t len=strlen(trimmed);
  if (len>0 && trimmed[len-1]==PATH_SEP) trimmed[len-1]=0;

  if (!hostfs_resolve(hostpaths[drive&DRVMSK],trimmed,hostname,sizeof(hostname))) {
    hostfs_setresult(HFSERRACS);
    return;
  };

#ifdef _WIN32
  if (_rmdir(hostname)==0) { hostfs_setresult(HFSERR_OK); return; }
  switch (errno) {
    case ENOTEMPTY: hostfs_setresult(HFSERRDNE); break;
    case ENOENT:    hostfs_setresult(HFSERRXDI); break;
    default:        hostfs_setresult(HFSERRACS); break;
  }
#else
  if (rmdir(hostname)==0) { hostfs_setresult(HFSERR_OK); return; }
  switch (errno) {
    case ENOTEMPTY: hostfs_setresult(HFSERRDNE); break;
    case ENOENT:    hostfs_setresult(HFSERRXDI); break;
    case ENOTDIR:   hostfs_setresult(HFSERRNDI); break;
    case EACCES:
    case EPERM:     hostfs_setresult(HFSERRACS); break;
    default:        hostfs_setresult(HFSERRGEN); break;
  }
#endif
};

static void hostfs_directory_move(void) {
/*
Directory_Move
  Moves a file or sub directory into another directory of the same drive. That means, that the drive
  letter of the source path must be the same like the drive letter of the destination path, otherwise
  the operation will be aborted. You can either move files or sub directories with this function, in 
  both cases the source path+name must not end with a "/".

I:H-CMD: ID for Directory_Move (041)
I:DAT#2: HostFS Drive to use

I:DAT#6: File/directory destination path address low
I:DAT#7: File/directory destination path address high
I:DAT#8: File/directory source path address low
I:DAT#9: File/directory source path address high
I:DAT#B: File/directory source and destination path address upper
*/
  uint8_t drive=hostfs_data[0x2];
  uint32_t z80srcptr=(hostfs_data[0xb]<<16)|(hostfs_data[0x9]<<8)|(hostfs_data[0x8]);
  uint32_t z80dstptr=(hostfs_data[0xb]<<16)|(hostfs_data[0x7]<<8)|(hostfs_data[0x6]);
  char *srcptr=(char*)&z80_mem[z80srcptr];
  char *dstptr=(char*)&z80_mem[z80dstptr];

  if (var_debug_hostfs>=1) printf("HOSTFS:%s:DRIVE %i:SRC #%s#:DST #%s#\n",__func__,drive,srcptr,dstptr);

  size_t srclen=strlen(srcptr);
  if (srclen>0 && (srcptr[srclen-1]=='/' || srcptr[srclen-1]=='\\')) {
    hostfs_setresult(HFSERRNAM); // source must not end with a separator
    return;
  }

  char srcresolved[PATH_MAX];
  if (!hostfs_resolve(hostpaths[drive&DRVMSK],srcptr,srcresolved,sizeof(srcresolved))) {
    hostfs_setresult(HFSERRACS);
    return;
  };
  char dstresolved[PATH_MAX];
  if (!hostfs_resolve(hostpaths[drive&DRVMSK],dstptr,dstresolved,sizeof(dstresolved))) {
    hostfs_setresult(HFSERRACS);
    return;
  };

  struct stat dst_st;
  char finaldst[PATH_MAX];
  if (stat(dstresolved,&dst_st)==0 && S_ISDIR(dst_st.st_mode)) {
    const char *base=strrchr(srcresolved,PATH_SEP);
    base = base ? base+1 : srcresolved;
    snprintf(finaldst,sizeof(finaldst),"%s%c%s",dstresolved,PATH_SEP,base);
  } else {
    strlcpy(finaldst,dstresolved,sizeof(finaldst));
  }

  struct stat final_st;
  if (stat(finaldst,&final_st)==0) {
    hostfs_setresult(HFSERRDBL);
    return;
  }

  if (rename(srcresolved,finaldst)==0) {
    hostfs_setresult(HFSERR_OK);
    return;
  }
#ifdef _WIN32
  hostfs_setresult(HFSERRACS);
#else
  if (errno==EXDEV) hostfs_setresult(HFSERRWDV); // different filesystem/device
  else if (errno==ENOENT) hostfs_setresult(HFSERRXDI);
  else hostfs_setresult(HFSERRACS);
#endif
};

static void hostfs_directory_driveinformation(void) {
/*
DriveInformation
  Returns information about one drive. Both Information Type 0 and Type 1 at the same time

I:H-CMD: ID for Directory_DriveInformation (042)
I:DAT#2: HostFS Drive to use

O:H-CMD: Status
O:DAT#1: Drive Status
O:DAT#2: Medium
O:DAT#3: File System
O:DAT#4: number of free 512byte sectors b0-b7
O:DAT#5: number of free 512byte sectors b8-b15
O:DAT#6: number of free 512byte sectors b16-b23
O:DAT#7: number of free 512byte sectors b24-b31
O:DAT#8: Total number of clusters b0-b7
O:DAT#9: Total number of clusters b8-b15
O:DAT#A: Total number of clusters b16-b23
O:DAT#B: Total number of clusters b24-b31
O:DAT#C: Sectors per cluster
*/
  uint8_t drive=hostfs_data[0x2];
  if (var_debug_hostfs>=1) printf("HOSTFS:%s:DRIVE %i\n",__func__,drive);

  uint8_t status=HFSDRV_NOTPRESENT, medium=HOSTFS_MEDIUM, filesystem=HOSTFS_FILESYSTEM, sectors_per_cluster=1;
  uint32_t free_sectors=0, total_clusters=0;

#ifdef _WIN32
  char root[PATH_MAX];
  strlcpy(root,hostpaths[drive&DRVMSK],sizeof(root));
  size_t rl=strlen(root);
  if (rl==0 || root[rl-1]!='\\') strlcat(root,"\\",sizeof(root));

  DWORD spc_w, bps_w, free_clusters_w, total_clusters_w;
  ULARGE_INTEGER free_avail, total_bytes, total_free;
  if (GetDiskFreeSpaceA(root,&spc_w,&bps_w,&free_clusters_w,&total_clusters_w) &&
      GetDiskFreeSpaceExA(root,&free_avail,&total_bytes,&total_free)) {
    status=HFSDRV_READY;
    unsigned long spc=spc_w*(bps_w/512 ? bps_w/512 : 1);
    if (spc<1) spc=1;
    sectors_per_cluster=(uint8_t)(spc>255?255:spc);
    unsigned long long fs=free_avail.QuadPart/512ULL;
    free_sectors=(uint32_t)(fs>0xFFFFFFFFULL?0xFFFFFFFF:fs);
    total_clusters=(uint32_t)(total_clusters_w);
    hostfs_setresult(HFSERR_OK);
  } else {
    hostfs_setresult(HFSERRDVP);
  }
#else
  struct statvfs sv;
  if (statvfs(hostpaths[drive&DRVMSK],&sv)==0) {
    status=HFSDRV_READY;
    unsigned long blocksize=sv.f_frsize?sv.f_frsize:sv.f_bsize;
    unsigned long spc=blocksize/512;
    if (spc<1) spc=1;
    sectors_per_cluster=(uint8_t)(spc>255?255:spc);
    unsigned long long fs=(unsigned long long)sv.f_bavail*spc;
    unsigned long long tc=(unsigned long long)sv.f_blocks;
    free_sectors=(uint32_t)(fs>0xFFFFFFFFULL?0xFFFFFFFF:fs);
    total_clusters=(uint32_t)(tc>0xFFFFFFFFULL?0xFFFFFFFF:tc);
    hostfs_setresult(HFSERR_OK);
  } else {
    hostfs_setresult(HFSERRDVP);
  }
#endif

  hostfs_data[0x1]=status;
  hostfs_data[0x2]=medium;
  hostfs_data[0x3]=filesystem;
  hostfs_data[0x4]=free_sectors;
  hostfs_data[0x5]=free_sectors>>8;
  hostfs_data[0x6]=free_sectors>>16;
  hostfs_data[0x7]=free_sectors>>24;
  hostfs_data[0x8]=total_clusters;
  hostfs_data[0x9]=total_clusters>>8;
  hostfs_data[0xA]=total_clusters>>16;
  hostfs_data[0xB]=total_clusters>>24;
  hostfs_data[0xC]=sectors_per_cluster;
};

void hostfs_execute(uint8_t cmd) {
  if (var_debug_hostfs>=2) {
    printf("HOSTFSDEBUG:%02X:",cmd);
    for (int i=1;i<16;i++) printf("%02X ",hostfs_data[i]);
    printf("\n");
  };

  switch(cmd) {
    case HFSID_File_New:
      hostfs_file_new();
      break;
    case HFSID_File_Open:
      hostfs_file_open();
      break;
    case HFSID_File_Close:
      hostfs_file_close();
      break;
    case HFSID_File_Input:
      hostfs_file_input();
      break;
    case HFSID_File_Output:
      hostfs_file_output();
      break;
    case HFSID_File_Pointer:
      hostfs_file_pointer();
      break;
    case HFSID_Directory_Property_Set:
      hostfs_directory_property_set();
      break;
    case HFSID_Directory_Property_Get:
      hostfs_directory_property_get();
      break;
    case HFSID_Directory_Rename:
      hostfs_directory_rename();
      break;
    case HFSID_Directory_New:
      hostfs_directory_new();
      break;
    case HFSID_Directory_Input:
      hostfs_directory_input();
      break;
    case HFSID_Directory_DeleteFile:
      hostfs_directory_deletefile();
      break;
    case HFSID_Directory_DeleteDirectory:
      hostfs_directory_deletedirectory();
      break;
    case HFSID_Directory_Move:
      hostfs_directory_move();
      break;
    case HFSID_Directory_DriveInformation:
      hostfs_directory_driveinformation();
      break;
    default:
      hostfs_setresult(HFSERRCMD);
      break;
  };
};

void init_hostfs(void) {
  hostpaths[0]=var_mount_folder_0;
  hostpaths[1]=var_mount_folder_1;
  hostpaths[2]=var_mount_folder_2;
  hostpaths[3]=var_mount_folder_3;
  hostpaths[4]=var_mount_folder_4;
  hostpaths[5]=var_mount_folder_5;
  hostpaths[6]=var_mount_folder_6;
  hostpaths[7]=var_mount_folder_7;
  getcwd(hostcwd,PATH_MAX);
};

void done_hostfs(void) {
  for (int fileid=0;fileid<MAX_FILES;fileid++) {
    if (hostfiles[fileid]!=NULL) {
      fclose(hostfiles[fileid]);
      hostfiles[fileid]=NULL;
    };
  };
};

/* vim: set et ts=2 sw=2 :*/
