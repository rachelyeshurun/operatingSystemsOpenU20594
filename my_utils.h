/*
* Operating Systems 20594 - Maman 13
* Rachel Cohen Yeshurun
*/

#ifndef MY_UTILS_H_
#define MY_UTILS_H_




/* Print contents of a directory to stdout */
/*int printDir(struct ext2_dir_entry_2 *pDirEntry, int inodeTable,int inodeSize,int blockSize);*/

/* Return 1 if a path exists, 0 if not. Path must begin with '/' example: /aa/bb/cc .*/
int isValidPath(char path[]);

#if 0
int updateCurrentDirectory(char *pDirName);
int getCurrentDirectory(char **ppDirName);

int get_disk_properties(int * block_size, struct ext2_super_block * sb, struct ext2_group_desc * gd);
#endif
#endif
