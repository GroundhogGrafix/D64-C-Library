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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <conio.h>
#include <math.h>
#include "CBMDisk.h"

// **************************FUNCTIONS**********************************

// Duplication routines
int cbmCopy(CBMDisk *dest, CBMDisk *source, byte *fileName, byte fileType)
{
    int err=0;
    CBMFile_Entry *entry=cbmSearch(source, fileName, fileType);
    CBMData *data;
    if(NULL==entry) return FILE_NOT_FOUND;
    data=cbmLoad(source, entry->fileName, entry->fileType);
    if(NULL==data) return ERROR;
    err=cbmSave(dest, (*entry).fileName, (*entry).fileType, data);            
    return err;
}

// Copy track 18 from a source file and store it in a destination file by itself.
int cbmCopy18(char *destName, char *sourceName)
{
    FILE *sourceFP, *destFP;
    byte tb[]={18,0};
    byte *maxSectors=cbmSectorsPerTrack();
    
    sourceFP=fopen(sourceName, "rb");
    destFP=fopen(destName, "wb");
    
    if(NULL==destFP) return FILE_ERROR;
    if(NULL==sourceFP) return FILE_ERROR;

    for(;tb[1]<maxSectors[tb[0]];tb[1]++)
        fwrite(cbmReadBlock(sourceFP, tb), BLOCK_SIZE, 1, destFP);
    
    fclose(sourceFP);
    fclose(destFP);

    return 0;    
}

int cbmWrite18(FILE *destFP, char *sourceName)
{
    FILE *sourceFP;
    byte sector[BLOCK_SIZE];
    byte tb[]={18,0};
    byte *maxSectors=cbmSectorsPerTrack();
    fflush(destFP);
    
    sourceFP=fopen(sourceName, "rb");
    if(NULL==sourceFP) return FILE_ERROR;

    for(;tb[1]<maxSectors[tb[0]];tb[1]++)
    {
        fread(sector, sizeof(sector), 1, sourceFP);
        cbmWriteBlock(destFP, sector, tb);
    }
        
    fclose(sourceFP);
    fclose(destFP);

    return 0;    
}

// Create new blank D64 image file OR erase an existing one.
int cbmFormat(char *imageName, byte *diskName, byte id)
{
    byte sector[BLOCK_SIZE];
    CBMDisk disk;
    byte tb[]={18,0};
    FILE *fp=fopen(imageName, "wb");
    int s=MAX_SECTORS;
        
    if(NULL==fp) return FILE_ERROR;

    memset(sector, 0, sizeof(sector));
    
    while(s--)
        fwrite(sector, sizeof(sector), 1, fp);

    cbmWrite18(fp, "Virgin18.CBM");

    fclose(fp);

    cbmMount(&disk, imageName);
    cbmCStringD64String(disk.header.diskName, diskName);
    disk.header.id[0]=id;
    fp=fopen(imageName, "r+b");
    cbmWriteBlock(fp, (byte*) &disk.header, tb);
    fclose(fp);
    cbmUnmount(&disk);

    return 0;        
}

// Image location routines
long cbmBlockLocation(byte *tb)
{
        // tb = 2 byte array, Track and Block to convert.
        // Return: Physical image location of the track and block conversion.
        FILE *fp;
        static long tl[DISK_TRACKS+1];
        char layout[]="layout.CBM";
        byte track=tb[0];
        byte block=tb[1];

        // Check to see if track locations have been loaded. If not then load them.
        if(!tl[1])
        {
                // Load image locations corresponding to each track.
                if((fp = fopen(layout, "rb"))==NULL)
                        return (long) FILE_ERROR;

                fread(tl, sizeof(long), DISK_TRACKS+1, fp);
                fclose(fp);
        }

        // Return track location+sector offset, this tells you exactly
        // where you need to read/write data to the disk image.
        return (long) (tl[track-1]+(block*BLOCK_SIZE));
}

byte* cbmSectorsPerTrack(void)
{
        // Create a list of the maximum sectors per track.
        // Returns an array of maximum sectors per track.
        static byte sectors[MAX_TRACKS+1];
        int a=1;
        int b=21;

        // Check to see if the list has already been created. If yes then return;
        if(sectors[1]) return sectors;

        // Create max sector per track list.
        for(;a<=MAX_TRACKS;a++)
        {
                if(18==a) b-=2;
                if(25==a) b--;
                if(31==a) b--;
                sectors[a]=b;
        }
        return sectors;
}

// Allocation routines
int cbmBAM(CBMDisk *disk, byte *tb, char s)
{
        // Three purpose function:
        // Check if a sector has been allocateded in the BAM.
        // Mark a sector allocated.
        // Mark a secotry free.
        // tb = 2 byte array, track and block to check or assign value.
        // s = 'r', 'a', 'f', r/read a/write or f/free.
        // Returns Zero for not available and non zero value for available sector.
        byte *bam=(*disk).header.bam;
        byte *freeSectors;
        unsigned int b=0;

        // If track sent to funtion is out of range, return out of range error.
        if(tb[0]>MAX_TRACKS) return OUT_OF_RANGE;

        // Multiply (track-1) by 4 to find start of track information in BAM
        // Every 4 bytes in the BAM represents all of the
        // sector informtion for each track.

        // Bytes per track correspond as follows:
        // 1st byte number of sectors free
        // 2nd byte sectors 0-7
        // 3rd byte sectors 8-15
        // 4th byte sectors 16-20, remaining bits unused.
        // Note: a 1 in the corresponding bit field
        // indicates an avilable sector and a 0 is allocated.
        bam+=(tb[0]-1)*4;
        freeSectors=bam;
        bam+=(tb[1]/8)+1;       // Increment BAM pointer to propper byte.
        b=_rotl(1,tb[1]);       // Rotate bit 1 left by number of sectors
        
        if(b>128) b=_rotl(b,8); // Integer to byte correction for propper comparison.
        if('a'==s)              // 'a' allocates a sector in BAM.
        {
            if(freeSectors[0]>0){
                b^=0xFF;
                bam[0]&=b;
                freeSectors[0]--;
            }
        }
        if('f'==s)  // 'f' frees a sector in BAM
        {              
            if(freeSectors[0]<0xFF){
                bam[0]|=b;
                freeSectors[0]++;
            }
        }                                
        
        return (int) (b&bam[0]);
}

int cbmIsBlockFree(CBMDisk *disk, byte *tb)
{
        return cbmBAM(disk, tb, 'r');
}

void cbmAllocateBlock(CBMDisk *disk, byte *tb)
{
        cbmBAM(disk, tb, 'a');
}

byte* cbmEmptyBlockChain(CBMDisk *disk, int size)
{
        byte *sectors=cbmSectorsPerTrack();
        int reqBlocks=(int)ceil((double)size/BLOCK_SIZE);
        byte *blocks=(byte*)malloc(sizeof(byte)*reqBlocks*2);
        
        int count=0;
        byte tb[2];
        tb[0]=1;
        tb[1]=0;
        fflush((*disk).fp);

        while(tb[0]<=MAX_TRACKS)
        {
                if(cbmIsBlockFree(disk, tb))
                {
                        blocks[count*2]=tb[0];
                        blocks[count*2+1]=tb[1];
                        count++;
                        if(count==reqBlocks) break;
                }
                tb[1]++;
                if(tb[1]==sectors[tb[0]]){
                        tb[0]+=(17==tb[0]) ? 2:1;
                        tb[1]=0;
                        }
        }
        if(count<reqBlocks) return NULL;
        return blocks;
}

// Mounting and Unmounting routines
void cbmMount( CBMDisk* disk, char *name )
{
        (*disk).imageName = name;
        (*disk).fp = fopen( name, "r+b" );
        cbmLoadHeader(disk);
        cbmLoadDirectory(disk);
}

void cbmUnmount(CBMDisk *disk)
{
        if((*disk).fp) fflush((*disk).fp);
        if(disk->directory.file) free(disk->directory.file);
        memset(disk, 0, sizeof(CBMDisk));
}
// Header routines
int cbmLoadHeader(CBMDisk *disk)
{
        byte tb[]={18,0};
        byte *buffer;

        fflush((*disk).fp);
        buffer = cbmReadBlock((*disk).fp, tb);

        memcpy(&(*disk).header, buffer, sizeof(CBMHeader));

        return 0;
}

// Directory routines
void cbmListDirectory(CBMDisk *disk)
{
        CBMDirectory *dir=&(*disk).directory;
        char ext[5][6]={".DEL", ".SEQ", ".PRG", ".USER", ".REL"};
        char fn[17];
        int a=0;

        for(a=0;a<(*dir).files;a++) {

                if((*dir).file[a].fileType)
                {
                cbmD64StringCString(fn,(*dir).file[a].fileName);
                printf("%3d %s",a+1, fn);
                printf("%s\n", ext[(*dir).file[a].fileType-0x80]);
                }
        }
}

CBMFile_Entry* cbmEmptyFileEntry(CBMDisk *disk)
{
        int a=0;

        for(a=0;a<(*disk).directory.files;a++)
                if(!(*disk).directory.file[a].fileType)
                        return (*disk).directory.file+a;

        return cbmAddDirectoryBlock(disk);
}

byte* cbmEmptyDirectoryBlock(CBMDisk *disk)
{
        static byte tb[2];
        byte *sectors=cbmSectorsPerTrack();
        tb[0]=18;
        tb[1]=1;

        for(tb[1]=1;tb[1]<sectors[tb[0]];tb[1]++)
                if(cbmIsBlockFree(disk, tb)) return tb;
              
        return NULL;
}

CBMFile_Entry* cbmAddDirectoryBlock(CBMDisk *disk)
{
        // disk=disk data
        // tb=Track and Block
        // db=Directory Block
        byte *tb=cbmEmptyDirectoryBlock(disk);
        CBMFile_Entry* db;
        int size=sizeof(CBMFile_Entry)*((*disk).directory.files+8);

        db=(CBMFile_Entry*) realloc((*disk).directory.file, size);
        if(db==NULL) return NULL;

        (*disk).directory.file=db;

        db+=(*disk).directory.files;
        memset(db, 0, sizeof(CBMFile_Entry)*8);
       
        (*disk).directory.file[(*disk).directory.files-8].nextBlock[0]=tb[0];
        (*disk).directory.file[(*disk).directory.files-8].nextBlock[1]=tb[1];
        (*db).nextBlock[0]=0;
        (*db).nextBlock[1]=0xFF;

        (*disk).directory.files+=8;

        return db;
}

int cbmLoadDirectory(CBMDisk *disk)
{
        byte tb[]={18,1};
        int size=sizeof(CBMFile_Entry)*8;
        CBMData *array=cbmLibReadBlockChain((*disk).fp, size, 0, tb);
        
        (*disk).directory.file=(CBMFile_Entry*) (*array).array;
        (*disk).directory.files=(*array).size/sizeof(CBMFile_Entry);

        return 0;
}

int cbmSaveDirectory(CBMDisk *disk)
{
    CBMFile_Entry *entry=(*disk).directory.file;
    byte firstBlock[]={18,1};
    byte *tb=firstBlock;
    
    do
    {
        cbmWriteBlock((*disk).fp, (byte*) entry->nextBlock, tb);
        tb=entry->nextBlock;
        entry+=8;
    }while(tb[0]);
   
    return 0;
}

int cbmSaveHeader(CBMDisk *disk)
{
    byte tb[]={18,0};

    if(NULL==disk->fp) return FILE_NOT_OPEN;
    cbmWriteBlock(disk->fp,(byte*) &disk->header, tb);
        
    return 0;
}

CBMFile_Entry* cbmCreateFileEntry(CBMDisk *disk, byte *fileName, byte fileType, unsigned int fileSize, byte *db)
{
    CBMFile_Entry *entry=cbmEmptyFileEntry(disk);
    if(NULL==entry) return NULL;

    cbmCStringD64String((*entry).fileName, fileName);
    (*entry).fileType=fileType;
    fileSize=(int)ceil((double)fileSize/BLOCK_SIZE);
    (*entry).fileSize[0]=(byte)(fileSize&0xFF);
    (*entry).fileSize[1]=(byte)((fileSize>>8)&0xFF);
    (*entry).dataBlock[0]=db[0];
    (*entry).dataBlock[1]=db[1];
    return entry;
}

// String routines
CBMFile_Entry* cbmSearch(CBMDisk *disk, byte *searchNameA, byte fileType)
{
    byte fileName[18];
    byte searchName[18];
    int a;
    
    cbmD64StringCString(searchName, searchNameA);
    
    for(a=0;a<(*disk).directory.files;a++)
    {
        cbmD64StringCString(fileName, (*disk).directory.file[a].fileName);
        if(!strcmp(searchName, fileName))
            if((*disk).directory.file[a].fileType==fileType)
                return (*disk).directory.file+a;
    }
    return NULL;
}

byte* cbmCopyString(byte *dest, const byte *source)
{
    int count=0;

    while(count<17)
    {
        if('\0'==source[count]) break;
        if(160==source[count]) break;
        dest[count]=source[count];
        count++;
    }

    return dest;
}
byte* cbmD64StringCString(byte *dest, const byte *source)
{
    int count=0;

    for(count=0;count<17;count++)
        dest[count]=' ';
    dest[count]='\0';

    return cbmCopyString(dest, source);
}

byte* cbmCStringD64String(byte *dest, const byte *source)
{
    int count=0;

    for(;count<17;count++)
        dest[count]=160;
            
    return cbmCopyString(dest, source);
}

// Data routines
CBMData* cbmLoad(CBMDisk *disk, byte *fileName, byte fileType)
{
    CBMFile_Entry *entry=cbmSearch(disk, fileName, fileType);
    if(NULL==entry) return NULL;

    return cbmLoadData((*disk).fp,(*entry).dataBlock);
}


CBMData* cbmLoadData(FILE *fp, byte *tb)
{
        return cbmLibReadBlockChain(fp, BLOCK_SIZE-2, 2, tb);
}

CBMData* cbmLibReadBlockChain(FILE *fp, int size, int offset, byte *tb)
{
        CBMData *retData=(CBMData*)malloc(sizeof(CBMData));
        byte *buffer=NULL;
        byte *data=NULL;
        int count=0;

        do
        {
                data=(byte*) realloc(data, size*(count+1));
                buffer = cbmReadBlock(fp, tb);
                tb=buffer;
                memcpy(data+(count*size), buffer+offset, size);
                count++;
        } while(*tb);

        (*retData).array=data;
        (*retData).size=(size*(count-1))+tb[1]-offset;
        return retData;
}

byte* cbmReadBlock(FILE *fp, byte *tb)
{
        static byte buffer[BLOCK_SIZE];
        fseek(fp, cbmBlockLocation(tb), SEEK_SET);

        fread(buffer, sizeof(byte), BLOCK_SIZE, fp);
        return buffer;
}

// Data write operations
int cbmSave(CBMDisk *disk, byte *fileName, byte fileType, CBMData *data)
{
    CBMFile_Entry *entry=cbmSearch(disk, fileName, fileType);
    byte *tb;
    if(NULL!=entry) return FILE_EXISTS;

    tb=cbmWriteBlockChain(disk, data);
    if(NULL==tb) return OUT_OF_MEMORY;
    memcpy((*entry).dataBlock, tb, 2);
    entry=cbmCreateFileEntry(disk, fileName, fileType, (*data).size, tb);
    if(NULL==entry) return OUT_OF_MEMORY;
    
    
    cbmSaveDirectory(disk);
    
    return 0;
}

void cbmWriteBlock(FILE *fp, byte *data, byte* tb)
{
        size_t val;
        fseek(fp, cbmBlockLocation(tb), SEEK_SET);
        val=fwrite(data, sizeof(byte), BLOCK_SIZE, fp);
}

byte* cbmWriteBlockChain(CBMDisk *disk, CBMData *data)
{
    int size=(*data).size;
    int ls;
    byte *dataPTR=(*data).array;
    byte *eb=cbmEmptyBlockChain(disk, size);
    int count=0;
    byte buffer[BLOCK_SIZE];
    
    if(NULL==eb) return NULL;
    fflush((*disk).fp);
    
    while((ls=(dataPTR-(*data).array))<size)
    {
        cbmBAM(disk, eb+(2*count), 'a');
        if((size-ls)<=BLOCK_SIZE-2){
            buffer[0]=0;
            buffer[1]=(byte)size-ls+2;
            }
            else {
                memcpy(buffer, &eb[2*(count+1)], 2);
            }
        
        memcpy(buffer+2, dataPTR, (BLOCK_SIZE-2));  
        cbmWriteBlock((*disk).fp, buffer, &eb[count*2]);
        dataPTR+=(BLOCK_SIZE-2);
        count++;        
    }
    cbmSaveHeader(disk);
    return eb;
}

void cbmPrintDataChain(FILE *fp, byte *tb)
{
        int blocks=0;
        long bytes=0;

        do
        {
                blocks++;
                printf("%3d) Track: %2d  Block: %2d          ", blocks, tb[0], tb[1]);
                tb=cbmReadBlock(fp, tb);
                if(!(blocks%2)) printf("\n");
        } while(tb[0]);
                
        printf("\n");
        printf("Total blocks: %3d    ", blocks);
        bytes=((BLOCK_SIZE-2)*(blocks-1))+tb[1]-2;
        printf("Data size in bytes: %5d    ", bytes);
        bytes=(BLOCK_SIZE*(blocks-1))+tb[1];
        printf("Size on disk: %5d\n", bytes);
}

// Deletion routines
int cbmScratch(CBMDisk *disk, byte *fileName, byte fileType)
{
    CBMFile_Entry *entry=cbmSearch(disk,fileName, fileType);
    byte *tb;
    if(NULL==entry) return FILE_NOT_FOUND;
    entry->fileType=CBMDEL;

    tb=entry->dataBlock;
    do
    {
        cbmBAM(disk, tb, 'f');
        fseek((*disk).fp, cbmBlockLocation(tb), SEEK_SET);
        fread(tb, sizeof(byte), 2, (*disk).fp);    
    }while(*tb);

    fwrite(&(*disk).header, sizeof(CBMHeader), 1, (*disk).fp);
        
    return 0;
}

// Disk information routines
int cbmImageSpace(CBMDisk *disk) // Returns amount of free space
{
        int a=0;
        int b=0;

        for(a=0;a<(4*35);a+=4)
                b+=(*disk).header.bam[a];
        return b;
}
