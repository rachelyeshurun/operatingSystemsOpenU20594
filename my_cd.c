/*
* Operating Systems 20594 - Maman 13
* Rachel Cohen Yeshurun
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

/*
typing my_cd <full_path_to_directory>
check if the directory exists on disk
If not, return with 1
If yes, write the full path to /tmp/.myext2
*/


int main(int argc, char *argv[])
{	
	char						directoryName[BUF_SIZE];
	int							fd;
		
	if (argc != 2)
	{
		printf("Usage: %s <directory name>\n", argv[0]);
		exit(1);
	}
	
	strcpy(directoryName, argv[1]);
	
	/* check if path valid without getting inner directory (NULL) */
	if (!isValidPath(directoryName, NULL))
	{
		printf("%s: File not found by ext2_lookup\n", directoryName);
		exit(1);
	}
	
	fd = open(CURRENT_PATH_FILE, O_RDWR | O_TRUNC) ;
	if (fd == -1)
	{
		if(errno == EACCES)
		{
			printf("[ERROR] open 'myext2' failed, permission denied\n");
		}
		else
		{
			/* last resort ..try to create it.. */
			fd = open(CURRENT_PATH_FILE, O_CREAT| O_RDWR | O_TRUNC, 0664);
			if (fd == -1)
			{
				printf("[ERROR] open 'myext2' failed\n");
				exit(1);
			}
		}
		exit(1);
	}
	int res = write(fd, directoryName, strlen(directoryName));
	
	if (res == -1)
	{
		printf("[ERROR] write directory to 'myext2' failed\n");
		close(fd);
		exit(1);
	}
	
	close(fd);
	return 0;
}

