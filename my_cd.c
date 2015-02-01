#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <linux/ext2_fs.h>
#include <linux/types.h>
#include "fcntl.h"
#include "unistd.h"
#include "errno.h"


#define DEBUG_MODE 
/* Note - must compile with -Wno-unused-value if debug mode off. Otherwise, get zillion warnings because of DBG macros create code with no effect */
#ifdef DEBUG_MODE
#define DBG_MSG		printf("\n[%d]: %s): ", __LINE__, __FUNCTION__);printf
#define DBG_ENTRY	printf("\n[%d]: --> %s", __LINE__,__FUNCTION__);
#define DBG_EXIT	printf("\n[%d]: <-- %s", __LINE__,__FUNCTION__);
#define DBG_DUMP(title) dumpHoard(title)
#else
#define DBG_MSG
#define DBG_ENTRY
#define DBG_EXIT
#define DBG_DUMP(title)
#endif


#define BUF_SIZE 2048
#define BYTES_PER_SECTOR 1024

static int readBlock(int blockNumber, char *buffer);
static int readInode(int inodeNumber, char *buffer);
static int getInode(struct ext2_inode* inodePointer, int inodeNumber);
static int getDirEntry( struct ext2_dir_entry_2 *pDirEntry, char* pEntryName, int inodeNumber);

/* given a directory path as string, split it on the '/'s and output an array of directory names */
static int splitPath(char* path, char*** result);

static int validPath(char path[]);

static int s_fid; 				/* global variable set by the open() function */
static int s_blockSize = BYTES_PER_SECTOR; 	/* bytes per sector from disk geometry */
static int s_inodeTable;
static int s_inodeSize;

int readBlock(int blockNumber, char *buffer)
{
	int dest, len;
	
	DBG_ENTRY
	
	s_fid = open ("/dev/fd0", O_RDWR);
	if (s_fid == -1)
	{
		printf("[ERROR] open failed\n");
		
		DBG_EXIT;
		return -1;
	}
	
	dest = lseek(s_fid, blockNumber * s_blockSize, SEEK_SET);
	if (dest != blockNumber * s_blockSize)
	{
		printf("[ERROR] lseek failed\n");
		close(s_fid);
		
		DBG_EXIT;
		return -1;
	}

	len = read(s_fid, buffer, s_blockSize);
	if (len != s_blockSize)
	{
		printf("[ERROR] read block failed\n");
		close(s_fid);
		
		DBG_EXIT;
		return -1;
	}
	close(s_fid);
	buffer[BYTES_PER_SECTOR - 1]= '\0';
	
	DBG_EXIT;
	return len;
}

int readInode(int inodeNumber, char *buffer)
{
	int dest, len, seekTo;
	
	s_fid = open ("/dev/fd0", O_RDWR);
	if (s_fid == -1)
	{
		printf("[ERROR] open failed\n");
		return -1;
	}
	
	seekTo = (s_inodeTable * s_blockSize) + (inodeNumber * s_inodeSize);
	DBG_MSG("seekTo=%d\n", seekTo);
	
	dest = lseek(s_fid,  seekTo, SEEK_SET);
	
	if (dest != seekTo)
	{
		printf("[ERROR] lseek failed\n");
		close(s_fid);
		return -1;
	}

	len = read(s_fid, buffer, s_inodeSize);
	if (len != s_inodeSize)
	{
		printf("[ERROR] read block failed\n");
		close(s_fid);
		return -1;
	}
	close(s_fid);
	return len;
}

int getInode(struct ext2_inode* inodePointer, int inodeNumber)
{
	char inode[s_inodeSize];

	int res = readInode(inodeNumber, inode);
	if (res == -1)
	{
		printf("[ERROR] read inode %d failed\n", inodeNumber);
		return -1;
	}
     if( memcpy((void*)inodePointer, inode, sizeof(struct ext2_inode))== NULL)
     {
    	 printf("[ERROR] memcpy to inode struct failed\n");
    	 return -1;
     }
	 
     return 0;
}

int getDirEntry( struct ext2_dir_entry_2 *pDirEntry, char* pEntryName, int inodeNumber)
{
	struct	ext2_inode 	inode;
    int 				i, j, dataBlockLength;
	char 				directoryDataArray[s_blockSize];

	if(getInode(&inode, inodeNumber)==-1)
	{
		printf("[ERROR] getInode failed\n");
		return -1;

	}
    DBG_MSG("modification time 
	for ( i = 0; i < 12 && (inode.i_block[i]!=0); i++ )
	{
		dataBlockLength = 0;
		dataBlockLength = readBlock(inode.i_block[i], directoryDataArray); /*first data block*/
		if(dataBlockLength == -1)
		{
			printf("[ERROR] getInode failed\n");
			return -1;

		}
		j = 0;
		while( j < dataBlockLength )
		{
			if( memcpy( pDirEntry, directoryDataArray + j, sizeof( struct ext2_dir_entry_2))!= NULL)
			{
				if(!strcmp(pDirEntry->name,pEntryName) && pDirEntry->file_type == 2)
				{
					return 0;
				}
			   j += pDirEntry->rec_len;
			}
		}
	}
    return -1; /*the entry wasn't found*/
}

int splitPath(char* path, char*** result)
{
	char**		ppSubDirectories;
	char* 		pEntryName= strtok(path, "/");
	int 		numEntries = 0;
	
	ppSubDirectories = (char**)malloc(sizeof(char*) * 1);
	while(pEntryName)
	{
		ppSubDirectories = realloc(ppSubDirectories, sizeof(char*) * (++numEntries + 1));
		if (ppSubDirectories == NULL)
		{
			printf("[ERROR] memory allocation (using realloc) failed\n");
			return -1;
		}
		ppSubDirectories[numEntries -1] = pEntryName;
		pEntryName= strtok(NULL, "/");
	}
	/*realloc one more entry for the NULL*/
	ppSubDirectories = realloc(ppSubDirectories, sizeof(char*) * (numEntries+1));
	ppSubDirectories[numEntries] = NULL;

	*result = ppSubDirectories;
	return 0;
}

int validPath(char path[])
{
	struct ext2_dir_entry_2		innerDirectory;
	struct ext2_dir_entry_2		currentDirectoryEntry;
	char**						ppSubDirectories;
	char						tmpPath[BUF_SIZE];
	int							i, inodeNumber = 1;
	strcpy(tmpPath , path);
	

	if(splitPath(tmpPath, &ppSubDirectories) == -1)
	{
		printf("[ERROR] split_path failed\n");
		return -1;
	}

	
	for(i=0; ppSubDirectories[i] != NULL; i++)
	{
		if (getDirEntry(&currentDirectoryEntry, ppSubDirectories[i], inodeNumber) == -1)
		{
			return -1;
		}
		inodeNumber = currentDirectoryEntry.inode - 1;
	}
	if( memcpy( &innerDirectory, &currentDirectoryEntry, sizeof( struct ext2_dir_entry_2 ) ) == NULL) /*the last one is the most inner one*/
	{
		printf("[ERROR] memcpy failed\n");
		return -1;
	}
	return 0;
}

int main(int argc, char *argv[])
{
	char						buffer[s_blockSize];	
	char						directoryName[BUF_SIZE];
	struct	ext2_super_block	superBlock;
	struct 	ext2_group_desc 	groupDescriptor;
	
	
	if (argc != 2)
	{
		printf("Usage: %s <directory name>\n", argv[0]);
		exit(1);
	}

	
	strcpy(directoryName, argv[1]);
	
	/* ---- SuperBlock ---- */
	int fd = readBlock(1, buffer);
	if (fd == -1)
	{
		printf("[ERROR] read super block failed\n");
		exit(1);
	}

	
	if (memcpy((void*)&superBlock, buffer, sizeof(struct ext2_super_block)) == NULL)
	{
		printf("[ERROR] memcpy super block failed\n");
		exit(1);
	}

	/*Block size expressed as power of 2 with 1024 as the unit*/
	s_blockSize = (int)pow(2, superBlock.s_log_block_size) * BYTES_PER_SECTOR; 
	
	/*Size of on-disk inode structure*/
	s_inodeSize = superBlock.s_inode_size; 
	DBG_MSG("s_inodeSize = %d", s_inodeSize);
	
	/* ---- GroupDescriptor ---- */
	fd = readBlock(2, buffer);
	if (f%dd == -1)
	{
		printf("[ERROR] read group descriptor block failed\n");
		exit(1);
	}
	
	
	if(memcpy((void*)&groupDescriptor, buffer, sizeof(struct ext2_group_desc)) == NULL)
	{
		printf("[ERROR] memcpy group descriptor block failed\n");
		exit(1);
	}
	
	s_inodeTable = groupDescriptor.bg_inode_table; /*Block number of first inode table block*/

	if ( validPath(directoryName) == 0 )
	{
		s_fid = open("/tmp/.myext2", O_RDWR | O_TRUNC) ;
		if (s_fid == -1)
		{
			if(errno == EACCES)
			{
				printf("[ERROR] open 'myext2' failed, permission denied\n");
			}
			else if(errno == ENONET)
			{
				printf("[ERROR] open 'myext2' failed, the file does not exist\n");
			}
			else
			{
				printf("[ERROR] open 'myext2' failed\n");
			}
			exit(1);
		}
		int res = write(s_fid, directoryName, strlen(directoryName));
		if (res == -1)
		{
			printf("[ERROR] write directory to 'myext2' failed\n");
			exit(1);
		}
	}
	else
	{
		printf("[ERROR] directory doesn't exist\n");
		exit(1);
	}
	
	/* TODO - move this to my_dir, extract common stuff etc. */
	
	/* implement my_dir */
	
	/* print to screen contents of s_fid */
	
	

	return 0;
}

