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


IMPORTANT: This file should only be used for examples of usage
and should not be included in your project. 

*/

#include<stdio.h>
#include<stdlib.h>
#include<conio.h>
#include<math.h>
#include "CBMDisk.h"

//*****************************MAIN FUNCTION*************************

int main()
{
        CBMDisk disk, disk2;
        CBMData *data;
        CBMFile_Entry *fileEntry;

        byte *test;
        byte tb2[2];
        byte *tb3;
                
        char dn[17];
        char string[20];
        unsigned int a=0;
        int b=0;
        // End Variable Declarations


        // Test Program Begin
        system("cls");
        // Test cbmFormat routine.
        printf("Test cbmFormat routine...\n");
        getch();
        cbmFormat("NewImage.D64", "TEST DISK", 0); 

        // Test cbmCopy18 routine, extracts track 18 from a .D64 image
        // and writes it to it's own file.
        printf("Test cbmCopy18 routine...\n");
        getch();
        cbmCopy18("Virgin18.CBM", "C:\\COMMODOR\\Blank.d64");
        
        // Test copy
        printf("Test copy...\n");
        getch();
        cbmMount(&disk, "C:\\COMMODOR\\KOALAPAD.D64");
        cbmMount(&disk2, "newimage.d64");
        cbmCopy(&disk2, &disk, "KOALA READER.BAS", CBMPRG);
        cbmCopy(&disk2, &disk, "PIC", CBMPRG);
        cbmListDirectory(&disk2);
        cbmUnmount(&disk);
        cbmUnmount(&disk2);
        
        // Test image mounting
        printf("Mount image...\n");
        getch();
        cbmMount(&disk, "newimage.d64");//"C:\\COMMODOR\\KOALAPAD.D64");
        printf("Success.\n");

        // Test Disk Header for accuracy
        printf("Print disk name located in header...\n");
        getch();
        cbmD64StringCString(dn, disk.header.diskName);
        printf("Disk name is %s\n", dn);

        getch();

        // Test Disk Directory for accuracy
        printf("Print a directory listing...\n");
        getch();
        cbmListDirectory(&disk);

        getch();

        // Test Disk File Data for accuracy
        printf("Print data loaded from file...\n");
        getch();
        data=cbmLoad(&disk, "KOALA READER.BAS", CBMPRG);
        for(a=0;a<data->size;a++)
        {
                printf("%c",data->array[a]);
        }
        printf("\n");
        getch();


        // Test Allocation Routines
        b=21*256;
        printf("Print a chain of free data locations on disk...\n");
        getch();
        test=cbmEmptyBlockChain(&disk, b);
        if(test==NULL){
                printf("Out of memory.");
                exit(0);
        }
        for(a=0;a<(int)ceil((double) (b/BLOCK_SIZE));a++)
        {
                printf("Free track, block %d, %d\n", test[a*2],test[a*2+1]);
        }
        free(test);
        getch();
        
        // Check for next available directory block.
        printf("Next available directory block...\n");
        getch();
        tb3=cbmEmptyDirectoryBlock(&disk);
        printf("Next empty directory block is: %d, %d\n", tb3[0], tb3[1]);
                
        // Debugging information for comparison to function accuracy
        printf("Print data chain from a file in the image...\n");
        getchar();
        cbmPrintDataChain(disk.fp, disk.directory.file[0].dataBlock);

        getch();
        printf("Test/Check track and block allocation in BAM...\n");
        printf("Enter letter e to end.\n");
        
        while(1)
        {
                printf("Input track\n");
                scanf("%s", string);
                if(string[0]=='e') break;
                tb2[0]=(byte) atoi(string);
                printf("Input block\n");
                scanf("%s", string);
                if(string[0]=='e') break;
                tb2[1]=(byte) atoi(string);

                if(cbmIsBlockFree(&disk, tb2)) printf("Free\n");
                        else printf("Allocated\n");
        }

        // Add a directory test.
        printf("Add a directory test...\n");
        getch();

        cbmAddDirectoryBlock(&disk); 
        cbmCStringD64String(disk.directory.file[disk.directory.files-1].fileName, "Test File       ");
        disk.directory.file[disk.directory.files-1].fileType=0x81;
        cbmListDirectory(&disk);
        getch();

        // Return first free directory entry
        printf("First free directory entry test...\n");
        getch();
        fileEntry=cbmEmptyFileEntry(&disk);
        cbmCStringD64String((*fileEntry).fileName, "First Free       ");
        (*fileEntry).fileType=0x82;
        cbmListDirectory(&disk);
        
        // Unmount disk
        printf("Unmount disk image...\n");
        getch();
        cbmUnmount(&disk);
        printf("Success.\n");
        getch();
        
        // End of program
        printf("End of test.\n");

        return 0;
}
