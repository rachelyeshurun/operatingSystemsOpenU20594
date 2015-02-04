/*
* Operating Systems 20594 - Maman 13
* Rachel Cohen Yeshurun
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <time.h>
#include <linux/fs.h>
#include <linux/types.h>
#include "fcntl.h"
#include "unistd.h"
#include "errno.h"
#include "my_utils.h"

#define _DEBUG_MODE 
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


#define BUF_SIZE 			2048			
#define BASE_BLOCK_SIZE		1024			/* base block size */
#define BASE_DISK_PATH 		"/dev/fd0" 		/* device top open*/

#define SUPERBLOCK_ADDR					1
#define GROUP_DESCRIPTORS_BLOCK_ADDR	2

/*
typing my_dir: program 
read current directory from /tmp/.myext2
print contents of directory (date, time name)
if /tmp/.myext2 does not exist, create it and print the contents (of this directory which is the root)
return 0 on success
return 1 on any file system error or if the given directory to print out is not found

typing my_cd <full_path_to_directory>
check if the directory exists on disk
If not, return with 1
If yes, write the full path to /tmp/.myext2
*/

/* set these values once per file system */
static void setBlockAndInodeSize(struct ext2_super_block superblock);

/* Read one block of data at given block number */
static int readBlock(int blockNumber, char *buffer);


static int readInode(int inodeNumber, char *buffer);


static int getInode(struct ext2_inode* inodePointer, int inodeNumber);


static int getDirEntry( struct ext2_dir_entry_2 *pDirEntry, char* pEntryName, int inodeNumber);

/* given a directory path as string, split it on the '/'s and output an array of directory names */
static int splitPath(char* path, char*** result);

/* print one directory entry (a file or another directory) */
int prettyPrintDirectoryEntry( struct ext2_dir_entry_2 dirEntry);
static int printInode(int inodeNumber);

/* Fixed per file system, can be 1024, 2048 or 4096. Default is 1024. */
static int s_blockSize = BASE_BLOCK_SIZE;
static int s_inodeSize = BASE_BLOCK_SIZE;

void setBlockAndInodeSize(struct ext2_super_block superblock)
{
	/*Block size expressed as power of 2 with 1024 as the unit*/
	s_blockSize = (int)pow(2, superblock.s_log_block_size) * BASE_BLOCK_SIZE; 
	/* 16bit value indicating the size of the inode structure.
	In revision 0, this value is always 128 (EXT2_GOOD_OLD_INODE_SIZE).
	In revision 1 and later, this value must be a perfect power of 2 and must be smaller
	or equal to the block size (1<<s_log_block_size). 
	*/
	s_inodeSize = superblock.s_inode_size;
}

int readBlock(int blockNumber, char *buffer)
{
	int seekTo, seekResult, readLen;
	int	file;
	
	DBG_ENTRY;
	
	file = open (BASE_DISK_PATH, O_RDWR);
	if (file == -1)
	{
		printf("[ERROR] open failed\n");		
		DBG_EXIT;
		return 0;
	}
	
	seekTo = blockNumber * s_blockSize;
	seekResult = lseek(file, seekTo, SEEK_SET);
	if (seekResult != seekTo)
	{
		printf("[ERROR] lseek failed\n");
		close(file);
		
		DBG_EXIT;
		return 0;
	}

	readLen = read(file, buffer, s_blockSize);
	if (readLen != s_blockSize)
	{
		printf("[ERROR] read block failed\n");
		close(file);
		
		DBG_EXIT;
		return 0;
	}
	close(file);
	
	/* TODO: potential bug here if given buffer is less than s_blockSize */
	buffer[s_blockSize - 1]= '\0';
	
	DBG_EXIT;
	return readLen;
}

int readInode(int inodeNumber, char *buffer)
{
	int							file;
	int 						seekResult, readLen, seekTo;
	struct ext2_group_desc 		groupDescriptor;
	char						groupDescriptorBuf[s_blockSize];
	
	DBG_ENTRY;
	file = open (BASE_DISK_PATH, O_RDWR);
	if (file == -1)
	{
		printf("[ERROR] open failed\n");
		DBG_EXIT;
		return 0;
	}
		
	/* read the first block of the group descriptors */
	if(!readBlock(GROUP_DESCRIPTORS_BLOCK_ADDR, groupDescriptorBuf))
	{
		printf("[ERROR] read group descriptor block failed\n");
		close(file);
		DBG_EXIT;
		return 0;
	}
	
	if(memcpy((void*)&groupDescriptor, groupDescriptorBuf, sizeof(struct ext2_group_desc)) == NULL)
	{
		printf("[ERROR] memcpy group descriptor block failed\n");
		close(file);
		DBG_EXIT;
		return 0;
	}
	DBG_MSG("readInode #%d", inodeNumber);	
	/* calculate start of inode block:
	 * inode table (first inode block number) multiplied by size of blocks plus
	 * inode number multiplied by size of inode blocks
	 */
	seekTo = (groupDescriptor.bg_inode_table * s_blockSize) + ((inodeNumber-1) * s_inodeSize);	
	
	seekResult = lseek(file,  seekTo, SEEK_SET);
	
	if (seekResult != seekTo)
	{
		printf("[ERROR] lseek failed\n");
		close(file);
		DBG_EXIT;
		return 0;
	}
	
	readLen = read(file, buffer, s_inodeSize);
	if (readLen != s_inodeSize)
	{
		printf("[ERROR] read block failed\n");
		close(file);
		DBG_EXIT;
		return 0;
	}
	
	close(file);
	DBG_EXIT;
	return readLen;
}

int getInode(struct ext2_inode* inodePointer, int inodeNumber)
{
	char inode[s_inodeSize];

	DBG_ENTRY;
	
	DBG_MSG("get inode # %d\n", inodeNumber);
	
	if (!readInode(inodeNumber, inode))
	{
		printf("[ERROR] read inode %d failed\n", inodeNumber);
		DBG_EXIT;
		return 0;
	}
	
	if( memcpy((void*)inodePointer, inode, sizeof(struct ext2_inode))== NULL)
	{
		printf("[ERROR] memcpy to inode struct failed\n");
		DBG_EXIT;
		return 0;
	}
	 DBG_EXIT;
	 return 1;
}

int getDirEntry( struct ext2_dir_entry_2 *pDirEntry, char* pEntryName, int inodeNumber)
{
	struct	ext2_inode 	inode;
    int 				i, j, dataBlockLength = 0;
	char 				directoryDataArray[s_blockSize];

	DBG_ENTRY;
	
	DBG_MSG("inodeNumber=%d inodePointer 0x%x %d\n", inodeNumber, (unsigned int)&inode, sizeof(struct ext2_inode));
	if(!getInode(&inode, inodeNumber))
	{
		printf("[ERROR] getInode failed\n");
		DBG_EXIT;
		return 0;

	}
		i=0;
	DBG_MSG("block num %d\n", inode.i_block[i]);
	for ( i = 0; i < 12 && (inode.i_block[i]!=0); i++ )
	{
		dataBlockLength = readBlock(inode.i_block[i], directoryDataArray); /*first data block*/
		if(!dataBlockLength)
		{
			printf("[ERROR] getInode failed\n");
			DBG_EXIT;
			return 0;
		}
		
		j = 0;
		while( j < dataBlockLength )
		{
			if( memcpy( pDirEntry, directoryDataArray + j, sizeof( struct ext2_dir_entry_2))!= NULL)
			{
				if(!strcmp(pDirEntry->name,pEntryName) && pDirEntry->file_type == 2)
				{
					DBG_EXIT;
					return 1;
				}
			   j += pDirEntry->rec_len;
			}
		}
	}
    DBG_EXIT;
	return 0; /*the entry wasn't found*/
}
int printInode(int inodeNumber)
{
	struct	ext2_inode 			inode;
    int 						i, j, dataBlockLength = 0;
	char 						directoryDataArray[s_blockSize];
	struct ext2_dir_entry_2 	dirEntry;

	DBG_ENTRY;
	
	DBG_MSG("inodeNumber=%d inodePointer 0x%x %d\n", inodeNumber, (unsigned int)&inode, sizeof(struct ext2_inode));
	if(!getInode(&inode, inodeNumber))
	{
		printf("[ERROR] getInode failed\n");
		DBG_EXIT;
		return 0;

	}
    
	i=0;
	DBG_MSG("block num %d\n", inode.i_block[i]);
	for ( i = 0; i < 12 && (inode.i_block[i]!=0); i++ )
	{
		
		DBG_MSG("block num %d\n", inode.i_block[i]);
		dataBlockLength = readBlock(inode.i_block[i], directoryDataArray); /*first data block*/
		if(!dataBlockLength)
		{
			printf("[ERROR] getInode failed\n");
			DBG_EXIT;
			return 0;
		}
		
		DBG_MSG("dataBlockLength=%d\n",dataBlockLength);
		
		j = 0;
		while( j < dataBlockLength )
		{
			if( memcpy( &dirEntry, directoryDataArray + j, sizeof( struct ext2_dir_entry_2))!= NULL)
			{
				DBG_MSG("dir entry name %s\n", dirEntry.name);
				if (!prettyPrintDirectoryEntry(dirEntry))
				{
					DBG_EXIT;
					return 0;
				}
				j += dirEntry.rec_len;
			}
		}
	}
    DBG_EXIT;
	return 1; 
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
			return 0;
		}
		ppSubDirectories[numEntries -1] = pEntryName;
		pEntryName= strtok(NULL, "/");
	}
	/*realloc one more entry for the NULL*/
	ppSubDirectories = realloc(ppSubDirectories, sizeof(char*) * (numEntries+1));
	ppSubDirectories[numEntries] = NULL;

	*result = ppSubDirectories;
	return 1;
}

int isValidPath(char path[], struct ext2_dir_entry_2 *pInnerDirectory)
{
	struct ext2_dir_entry_2		currentDirectoryEntry;
	char**						ppSubDirectories;
	char						tmpPath[BUF_SIZE];
	int							i, inodeNumber = 2;
	struct	ext2_super_block	superBlock;
	
	DBG_ENTRY;
	strcpy(tmpPath , path);
	
	if(!splitPath(tmpPath, &ppSubDirectories))
	{
		printf("[ERROR] split_path failed\n");
		DBG_EXIT;
		return 0;
	}

	if (!getSuperblock(&superBlock))
	{
		printf("[ERROR] read super block failed\n");
		DBG_EXIT;
		return 0;
	}
	
	setBlockAndInodeSize(superBlock);
	DBG_MSG("Next inodeNumber %d",inodeNumber);
	for(i=0; ppSubDirectories[i] != NULL; i++)
	{
		if (!getDirEntry(&currentDirectoryEntry, ppSubDirectories[i], inodeNumber)  )
		{
			DBG_EXIT;
			return 0;
		}
		DBG_MSG("currentDirectoryEntry.name %s currentDirectoryEntry.inode %d\n", currentDirectoryEntry.name, currentDirectoryEntry.inode);
		inodeNumber = currentDirectoryEntry.inode;
		DBG_MSG("Next inodeNumber %d",inodeNumber);
	}
	
	/* return inner directory if requested */
	if (pInnerDirectory)
	{
		if( memcpy( pInnerDirectory, &currentDirectoryEntry, sizeof( struct ext2_dir_entry_2 ) ) == NULL)
		{
			printf("[ERROR] memcpy failed\n");
			DBG_EXIT;
			return 0;
		}
	}
		
	DBG_EXIT;
	return 1;
}

int getSuperblock(struct ext2_super_block	*pSuperBlock)
{
	char						buffer[s_blockSize];
	
	DBG_ENTRY;
	
	/* Read in the superblock */
	if (!readBlock(SUPERBLOCK_ADDR, buffer))
	{
		printf("[ERROR] read super block failed\n");
		DBG_EXIT;
		return 0;
	}
	
	if (memcpy((void*)pSuperBlock, buffer, sizeof(struct ext2_super_block)) == NULL)
	{
		printf("[ERROR] memcpy super block failed\n");
		DBG_EXIT;
		return 0;
	}
	
	return 1;

}

/* Print contents of a directory to stdout. Path must begin with '/' example: /aa/bb/cc .*/
int printDirectory(char path[])
{
	struct	ext2_inode			inode;
	struct	ext2_dir_entry_2	innerDirectory;
	int							inodeNum;
	
	DBG_ENTRY;
	
	isValidPath(path, &innerDirectory);	

	inodeNum = innerDirectory.inode;
	
	if (!inodeNum )
	{
		/* inode num is weird - give the root directory (inode 1) TODO - why does it work with 2 and not 1? */
		inodeNum = 2;
	}

	if(!printInode(inodeNum))
	{
		DBG_EXIT;
		return 0;
	}
		
	DBG_EXIT;
	return 1;
}

int prettyPrintDirectoryEntry( struct ext2_dir_entry_2 dirEntry)
{
	struct tm  				*pTimeInfo;
    char       				timeString[BUF_SIZE];
	char       				nameString[BUF_SIZE];
	struct ext2_inode 		inode;
	
	DBG_ENTRY;
	if(!getInode(&inode, dirEntry.inode))
	{
		DBG_EXIT;
		return 0;
	}
	
	/* print the time like this: 18-Feb-1970 09:29         */
	pTimeInfo = localtime((time_t *) &inode.i_mtime);
    strftime(timeString, sizeof(timeString), "%b-%Y %H:%M", pTimeInfo);
	
	strncpy(nameString, dirEntry.name, dirEntry.name_len);
	nameString[dirEntry.name_len] = '\0';
    printf(" %2d-%s %s\n", pTimeInfo->tm_mday,timeString, nameString);
	
	DBG_EXIT;
	return 1;
}
