#ifndef _HOSTFS_H
#define _HOSTFS_H

#define ATTR_READONLY  0x01
#define ATTR_HIDDEN    0x02
#define ATTR_SYSTEM    0x04
#define ATTR_VOLUME    0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE   0x20

#define HFSID_File_New                   17
#define HFSID_File_Open                  18
#define HFSID_File_Close                 19
#define HFSID_File_Input                 20
#define HFSID_File_Output                21
#define HFSID_File_Pointer               22
#define HFSID_Directory_Property_Set     34
#define HFSID_Directory_Property_Get     35
#define HFSID_Directory_Rename           36
#define HFSID_Directory_New              37
#define HFSID_Directory_Input            38
#define HFSID_Directory_DeleteFile       39
#define HFSID_Directory_DeleteDirectory  40
#define HFSID_Directory_Move             41
#define HFSID_Directory_DriveInformation 42

#define HFSERR_OK   1      //OK
#define HFSERRMIS   0      //Device not present
#define HFSERRUNI   2      //Device not initialized
#define HFSERRFAI   3      //Media defective
#define HFSERRPNO   4      //Partition not found
#define HFSERRPTP   5      //Media or partition not supported
#define HFSERRSEC   6      //Error reading/writing sector
#define HFSERRPOS   7      //Positioning error
#define HFSERRABO   8      //Aborted during media access
#define HFSERRUNK   9      //Unknown media error
#define HFSERRHND   10     //No file handler available
#define HFSERRXDV   11     //Device does not exist (for path selection)
#define HFSERRXDI   12     //Path does not exist (for path selection)
#define HFSERRXFI   13     //File does not exist (for path selection)
#define HFSERRACS   14     //Access denied
#define HFSERRNAM   15     //Invalid file, wildcard, or path name
#define HFSERRXHN   16     //Handler does not exist
#define HFSERRDEV   17     //Device slot already occupied
#define HFSERRREA   18     //Error reading file (File organization)
#define HFSERRNNA   19     //Invalid destination name (incorrect or a path was specified)
#define HFSERRDBL   20     //(Destination) name already exists
#define HFSERRSCM   21     //Incorrect sub-command code
#define HFSERRATR   22     //Invalid attribute
#define HFSERRDFL   23     //No free space in directory
#define HFSERRSFL   24     //No space left on device
#define HFSERRWRP   25     //Write protection enabled, cannot write to media
#define HFSERRRDY   26     //Device not ready
#define HFSERRDNE   27     //Directory is not empty and cannot be deleted
#define HFSERRWDV   28     //Target device is not identical to source device
#define HFSERRSUP   29     //Command not supported for this file system
#define HFSERRDVP   30     //Device not supported
#define HFSERRRDO   31     //File is ReadOnly
#define HFSERRXCH   32     //Device channel not available
#define HFSERRNDI   33     //Target is not a directory
#define HFSERRNFI   34     //Target is not a file
#define HFSERRCMD   35     //Unsupported command
#define HFSERRGEN   255    //Undefined error

#define HFSPROP_ATTRIBUTE 0
#define HFSPROP_MODIFIED  1
#define HFSPROP_CREATED   2

#define HFSDRV_NOTPRESENT 0
#define HFSDRV_READY      1
#define HOSTFS_FILESYSTEM 18 //FAT32
#define HOSTFS_MEDIUM	  16 //HARDDISC

extern uint8_t hostfs_data[16];

void hostfs_execute(uint8_t cmd);
int hostfs_resolve(char *root, char *guest, char *out, size_t outsz);
void init_hostfs(void);
void done_hostfs(void);

#endif
