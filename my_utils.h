/*
* Operating Systems 20594 - Maman 13
* Rachel Cohen Yeshurun
*/

#ifndef MY_UTILS_H_
#define MY_UTILS_H_


#include <linux/ext2_fs.h>

#define CURRENT_PATH_FILE 		"/tmp/.myext2" 	/* File to store the current working directory, instead of PWD ENV*/

/* Return 1 if a path exists, 0 if not. Path must begin with '/' example: /aa/bb/cc .
 * Return inner directory if pInnerDirectory not NULL.
*/
int isValidPath(char path[], struct ext2_dir_entry_2 *pInnerDirectory);

/* Print contents of a directory to stdout. Path must begin with '/' example: /aa/bb/cc .*/
int printDirectory(char path[]);

#endif
