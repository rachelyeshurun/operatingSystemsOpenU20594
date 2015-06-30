/*
* Operating Systems 20594 - Maman 13
* Rachel Cohen Yeshurun
*
* This belongs to the Local Repository
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <linux/ext2_fs.h>
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


#define BUF_SIZE 2048
/*
typing my_dir: program 
read current directory from /tmp/.myext2
print contents of directory (date, time name)
if /tmp/.myext2 does not exist, create it and print the contents (of this directory which is the root)
return 0 on success
return 1 on any file system error or if the given directory to print out is not found
 */

int main(int argc, char *argv[])
{	
	char						directoryName[BUF_SIZE];
	int							fileDesc;
	int							readLen;
		
	if (argc != 1)
	{
		printf("Usage: %s\n", argv[0]);
		exit(1);
	}
	
	fileDesc = open(CURRENT_PATH_FILE, O_RDONLY) ;	
	if (fileDesc == -1)
	{
		if(errno == EACCES)
		{
			printf("[ERROR] open 'myext2' failed, permission denied\n");
			exit(1);
		}
		else
		{
			/*open 'myext2' failed/the file does not exist. Create it.*/
			fileDesc = open(CURRENT_PATH_FILE, O_CREAT| O_RDWR | O_TRUNC, 0664);
			if (fileDesc == -1)
			{
				printf("[ERROR] open 'myext2' failed and the file cannot be created.\n");
				exit(1);
			}
			readLen = write(fileDesc, "/.", strlen("/."));
			if (readLen == -1)
			{
				printf("[ERROR] write root to 'myext2' failed\n");
				close(fileDesc);
				exit(1);
			}

		}
	}
	
	readLen = read(fileDesc, directoryName, BUF_SIZE);
	if (readLen == -1)
	{
		printf("[ERROR] read path file failed\n");
		close(fileDesc);		
		exit(1);
	}
	
	close(fileDesc);

	directoryName[readLen]= '\0';

	/* Print contents of a directory to stdout. Path must begin with '/' example: /aa/bb/cc .*/
	if (!printDirectory(directoryName))
	{
		printf("[ERROR] could not print directory\n");		
		exit(1);
	}
	return 0;
}

