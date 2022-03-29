/*
D64 Disk Utilities Version 1.0

Author: Martin Sigley                         Date: 4/11/2018

Disclaimer: I will NOT be held liable for any damages resulting
from the use of this software. Not sure what those damages
could be but I'll cover my ass anyway. I hope that this benefits
the community in some way and feel free to improve on it as you
wish, please just give credit where credit is due for the
original creation.

License: The library was tailored to my own personal needs and
created out of the fondness I have for the C64. This is FREEWARE
and you can modify it, put it in commercial software and do
anything you like as long as you give credit to the author,
Martin Sigley, the original creater of this library.

IMPORTANT: This version is very much in beta form and is not
very well tested yet, I have tested the routines only to a
limited extent.

DO NOT USE THIS WITH D64 IMAGES THAT YOU ONLY HAVE ONE COPY OF,
YOU MIGHT REGRET IT!

If you should find errors please contact me at the email listed
below.

Sincerely,
Martin Sigley
remove-this-for-spam-protection-firebreathinggroundhog@yahoo.com
*/

#define MAX_OPEN_FILES 10
#define BLOCK_SIZE 256
#define DIR_ENTRIES 8
#define DISK_TRACKS 40
#define FILE_NAME_WIDTH 16
#define BAM_SIZE 4*35
#define MAX_TRACKS 35
#define FALSE 0
#define MAX_SECTORS 683
#define CBMDEL 0x80
#define CBMSEQ 0x81
#define CBMPRG 0x82
#define CBMUSR 0x83
#define CBMREL 0x84

// ERROR CODES
#define FILE_ERROR      -1
#define OUT_OF_MEMORY   -2
#define OUT_OF_RANGE    -3
#define FILE_NOT_FOUND  -4
#define ERROR           -5
#define FILE_NOT_OPEN   -6
#define FILE_EXISTS     -7

typedef unsigned char   byte;                       // New data type BYTE.

typedef struct
{
        byte            nextBlock[2];
        byte*           array;
} CBMBlock;

typedef struct
{
        byte*           array;
        unsigned int    size;
} CBMData;

typedef struct CBMHeader
{
        byte            nextBlock[2];
        byte            dosVersion;
        byte            unused1;
        byte            bam[BAM_SIZE];
        byte            diskName[16];
        byte            id[2];
        byte            unused2;                    // Usually $A0
        byte            dosType[2];
        byte            unused3[4];                 // Usually $A0
        byte            track40Extended[55];        // Usually $00, except for 40 track format
} CBMHeader;

typedef struct
{
        byte            nextBlock[2];               // Track and block of next directory
                                                    // block. When the first byte is 00
                                                    // that is the last block.             

        byte            fileType;
                                                    // 0x80 = DELeted                       
                                                    // 0x81 = SEQuential                    
                                                    // 0x82 = PROGram
                                                    // 0x83 = USER
                                                    // 0x84 = RELative

        byte            dataBlock[2];               // Track and block of first data block
        byte            fileName[16];               // Filename padded with spaces
        byte            sideSector[2];              // Relative only track and block first side
                                                    // sector.                             

        byte            recordSize;                 // Relative file only. Record Size.                     
        byte            unused[6];                  // Unused bytes                                        

        byte            fileSize[2];                // Number of blocks in file. Low Byte, High Byte.             
} CBMFile_Entry;

typedef struct
{
        CBMFile_Entry   *file;
        int             files;
} CBMDirectory;

typedef struct
{
        char            *imageName;
        FILE            *fp;

        CBMHeader       header;
        CBMDirectory    directory;
} CBMDisk;

// **************************FUNCTION DEFINITIONS******************************

// String routines
        byte*           cbmCopyString               (byte *dest, const byte *source);
        byte*           cbmD64StringCString         (byte *dest, const byte *source);
        byte*           cbmCStringD64String         (byte *dest, const byte *source);
        CBMFile_Entry*  cbmSearch                   (CBMDisk *disk, byte *searchNameA, byte fileType);

// Image location routines
        long            cbmBlockLocation            (byte *tb);
        byte*           cbmSectorsPerTrack          (void);

// Data read routines
        CBMData*        cbmLibReadBlockChain        (FILE *fp, int size, int offset, byte *tb);
        byte*           cbmReadBlock                (FILE *fp, byte *tb);
        CBMData*        cbmLoadData                 (FILE *fp, byte *tb);
        CBMData*        cbmLoad                     (CBMDisk *disk, byte *fileName, byte fileType);

// Data write operations
        byte*           cbmWriteBlockChain          (CBMDisk *disk, CBMData *data);
        int             cbmSave                     (CBMDisk *disk, byte *fileName, byte fileType, CBMData *data);
        void            cbmWriteBlock               (FILE *fp, byte *data, byte* tb);

// File deletion
        int             cbmScratch                  (CBMDisk *disk, byte *fileName, byte fileType);

// Disk information routines
        int             cbmImageSpace               (CBMDisk *disk);

// Allocation routines
        int             cbmBAM                      (CBMDisk *disk, byte *tb, char s);
        int             cbmIsBlockFree              (CBMDisk *disk, byte *tb);
        void            cbmAllocateBlock            (CBMDisk *disk, byte *tb);
        byte*           cbmEmptyBlockChain          (CBMDisk *disk, int size);
        void            cbmPrintDataChain           (FILE *fp, byte *tb);

// Directory routines
        byte*           cbmEmptyDirectoryBlock      (CBMDisk *disk);
        int             cbmLoadDirectory            (CBMDisk*);
        void            cbmListDirectory            (CBMDisk*);
        CBMFile_Entry*  cbmAddDirectoryBlock        (CBMDisk *disk);
        CBMFile_Entry*  cbmEmptyFileEntry           (CBMDisk *disk);
        CBMFile_Entry*  cbmCreateFileEntry          (CBMDisk *disk, byte *fileName, byte fileType, unsigned int fileSize, byte *tb);

// Header routines
        int             cbmLoadHeader               (CBMDisk*);
        int             cbmSaveHeader               (CBMDisk *disk);

// Mounting routines
        void            cbmMount                    (CBMDisk* disk, char *name );
        void            cbmUnmount                  (CBMDisk *disk);

// Duplication routines
        int             cbmCopy                     (CBMDisk *dest, CBMDisk *source, byte *fileName, byte fileType);
        int             cbmCopy18                   (char *destName, char *sourceName);
        int             cbmWrite18                  (FILE *destFP, char *sourceName);

// Format
        int             cbmFormat                   (char *imageName, byte *diskName, byte id);
