#include <stdlib.h>
#include <stdio.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <unistd.h>
#include "types.h"
#include "fs.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#define T_DIR 1
#define T_FILE 2
#define T_DEV 3


typedef struct superblock superblock;
typedef struct dinode dinode;
typedef struct dirent dirent;

struct superblock* sblock2;
struct dinode* inode_ptr2;

int *inode_used;
int *inode_used_directory;

int *block_used_direct;

int *block_used_indirect;
//int *block_marked_bitmap;
char* bitmap_ptr;
void *image_ptr;

int check_bitmap(char* bitmap_ptr, uint addr);
void  check_directory(void *image_ptr);

int main (int argc, char *argv[]) {

	//check usage
	if (argc != 2) {
		printf("Usage: xcheck <file_system_image>\n");
		exit(1);
	}
	int fd = open(argv[1], O_RDONLY);
       	if (fd < 0) {
		//printf("Usage: xcheck <file_system_image>\n");
		fprintf(stderr, "image not found.\n");	
		exit(1);
	}

	//get metadata
        struct stat image_stat;
        if (fstat(fd, &image_stat) == 0) {
                image_ptr = mmap(NULL, image_stat.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
        }
//	metadata_t mdata;
//	get_metadata(&mdata, fd);

	
	//TESTING
/*	
 	printf("FS Size: %u, Num Blocks: %u, Num Inodes: %u\n",
			mdata.sblock.size, mdata.sblock.nblocks, mdata.sblock.ninodes);

	int inum = 2;
	printf("Inode #%d. type = %hd, num links = %hd, size = %u\n",
			inum, mdata.inodes[inum].type,
			mdata.inodes[inum].nlink, mdata.inodes[inum].size); 
*/
	sblock2 = (struct superblock*) (image_ptr + BSIZE);		
//initlaized to be zero
	inode_used = malloc(sblock2->ninodes*sizeof(int));
	for (int i = 0; i < sblock2->ninodes; i++) {
                inode_used[i] = 0;
        }

	block_used_direct = malloc(sblock2->nblocks*sizeof(int));
	block_used_indirect = malloc(sblock2->nblocks*sizeof(int));		
//	block_marked_bitmap = malloc(sblock2->nblocks*sizeof(int));
	
	for (int i = 0; i < sblock2->nblocks; i++) {
		//initalized to be 0
		block_used_direct[i] = 0;
		block_used_indirect[i] = 0;
//		block_marked_bitmap[i] = 0;
	}
	
	bitmap_ptr = image_ptr + (sblock2->ninodes/IPB + 3) *BSIZE;
	
	inode_ptr2 = (struct dinode*) (image_ptr + 2*BSIZE);
        //TESTING
     	/* 
        printf("FS Size: %u, Num Blocks: %u, Num Inodes: %u\n",
                        sblock2->size, sblock2->nblocks, sblock2->ninodes);
	*/
        /*int inum = 2;
        printf("Inode #%d. type = %hd, num links = %hd, size = %u\n",
                        inum, mdata.inodes[inum].type,
                        mdata.inodes[inum].nlink, mdata.inodes[inum].size); 
	*/



	struct dinode *current_inode = inode_ptr2;  	
	for (int i = 0; i < sblock2->ninodes; i++) {	
		if (current_inode->type != 0 
		    && current_inode->type != T_DIR 
		    && current_inode->type != T_DEV
		    && current_inode->type != T_FILE) {
			fprintf(stderr, "ERROR: bad inode.\n");
			exit(1);
		} else {	//in-use, allocated inode
			if (current_inode->type != 0) {
				inode_used[i] = inode_used[i] + 1;
			}

			if (i == 1 ) {
				if (current_inode->type != T_DIR) {
                                	fprintf(stderr, "ERROR: root directory does not exist.\n");
                                	exit(1);
				}
				

                        } 

			if (current_inode->type == T_DIR) {
 				struct dirent *dirent_init;	
				//check "." and ".." 
        			dirent_init = (struct dirent*) (image_ptr + current_inode->addrs[0] *BSIZE);
				if (strcmp(dirent_init->name, ".") || strcmp((dirent_init + 1)->name, "..")) {
       					fprintf(stderr, "ERROR: directory not properly formatted.\n");
					exit(1);
				}
				if (i == 1) {
					//check if parent is itslef
					if ((dirent_init+1)->inum != i) {
						fprintf(stderr, "ERROR: root directory does not exist.\n");
						exit(1);
					}
				}	
			
			}

			uint block_num;		
			for (int j = 0; j < NDIRECT+1; j++) {				
				block_num = current_inode->addrs[j];
							
				if (block_num * BSIZE > image_stat.st_size) {
					fprintf(stderr, "ERROR: bad direct address in inode.\n");
					exit(1);
				}

				if (block_num > 0) {
					block_used_direct[block_num] = block_used_direct[block_num] + 1;
					block_used_indirect[block_num] = block_used_indirect[block_num] + 1;
				        if (!check_bitmap(bitmap_ptr, block_num)) {
                                                fprintf(stderr, "ERROR: address used by inode but marked free in bitmap.\n");
                                                exit(1);
                                        }
				}	
			}
			
				//there is a valid indirect pointe
				
			uint* indirect_ptr = (uint*)(image_ptr + current_inode->addrs[NDIRECT]*BSIZE);
			for (int h = 0; h < (BSIZE/sizeof(uint)); h++) {
				block_num = (uint) *indirect_ptr;
			        if ((block_num *BSIZE) >image_stat.st_size) {
                                        fprintf(stderr, "ERROR: bad indirect address in inode.\n");
                                        exit(1);
                                }

				if (block_num > 0 ) {
					block_used_indirect[block_num] = block_used_indirect[block_num] + 1;	
				}
				indirect_ptr++;
			}

	         }//end of else

		current_inode++;
	}	

	check_directory(image_ptr);

	for  (int i = 0; i < sblock2->nblocks; i++) {
		if (block_used_direct[i] > 1) {
 			fprintf(stderr, "ERROR: direct address used more than once.\n");
			exit(1);
		}	
		if (block_used_indirect[i] > 1) {
			//fprintf(stderr, "ERROR: indirect address used more than once.\n");
                        fprintf(stderr, "ERROR: bad indirect address in inode.\n");
			exit(1);
		}
		if ((block_used_indirect[i] == 0) && (check_bitmap(bitmap_ptr, i)==1)) {
			// fprintf(stderr, "ERROR: bitmap marks block in use but it is not in use.");
			//exit(1);
			
		}
	}	
	

	dinode* temp = (struct dinode*) (image_ptr + 2 *BSIZE);
	for (int i = 0; i < sblock2->ninodes;i++) {		
		if (temp->type == T_DIR) {
			if (inode_used_directory[i] > 1) {
				fprintf(stderr, "ERROR: directory appears more than once in file system.\n");
				exit(1);	
		 	}	
		}
		if (temp->type != 0) {
			if (temp->nlink != inode_used_directory[i]) {
				fprintf(stderr, "ERROR: bad reference count for file.\n");
				exit(1);
			}
		}
		if (inode_used[i] >= 1 && inode_used_directory[i] ==0) {
			fprintf(stderr, "ERROR: inode marked use but not found in a directory.\n");
			exit(1);	
		}
		if (inode_used[i] == 0 && inode_used_directory[i] >=1) {
			fprintf(stderr, "ERROR: inode referred to in directory but marked free.\n");
			exit(1);
		}
		temp++;	
	}
	//cleanup
	close(fd);
	//free metadata
	munmap(image_ptr, image_stat.st_size);

	return 0;
}


/*
void get_superblock(superblock *sblock, int fd) {
	void *mapped = mmap(NULL, (2 * BSIZE), PROT_READ, MAP_PRIVATE, fd, 0);
	superblock *temp = mapped + BSIZE;

	sblock->size = temp->size;
	sblock->nblocks = temp->nblocks;
	sblock->ninodes = temp->ninodes;

	munmap(mapped, (2 * BSIZE));
}

void get_metadata(metadata_t *mdata, int fd) {
	//get superblock info
	get_superblock(&mdata->sblock, fd);
	//get number of inode blocks
	int num_iblocks = (mdata->sblock.ninodes / IPB);
	
	if (mdata->sblock.ninodes % IPB)
		num_iblocks += 1;
	//get number of bitmap blocks
	int num_bblocks = (mdata->sblock.nblocks / BPB);
	if (mdata->sblock.nblocks % BPB)
		num_bblocks += 1;
	//calc total size
	mdata->size = (2 * BSIZE) + (num_iblocks * BSIZE) + (num_bblocks * BSIZE);
	//map metadata
	mdata->ptr = mmap(NULL, mdata->size, PROT_READ, MAP_PRIVATE, fd, 0);
	//set inodes and bitmap ptrs
	mdata->inodes = (dinode *) (mdata->ptr + (2 * BSIZE));
	//mdata->bitmap = ((void *) mdata->inodes) + (num_iblocks * BSIZE);
	mdata->bitmap = (char *)(mdata->ptr + (num_iblocks + 3) * BSIZE);
}
*/


int check_bitmap(char *bitmap_ptr, uint addr) {	
	char byte = *(bitmap_ptr + addr/8); //find the bitmap_byte location
	if ((byte >> (addr % 8)) & 0x01)
		return 1; //marked used
	return 0;

}


void check_directory(void *image_ptr) {
	//return the number of times current_inode is referred in directory
	struct superblock *sb = (struct superblock*)(image_ptr + BSIZE);	
	struct dinode *current_inode_ptr = (struct dinode*)(image_ptr + 2*BSIZE);
	struct dirent* dirent;
	inode_used_directory = malloc(sb->ninodes*sizeof(int));
	for (int i = 0; i < sb->ninodes; i++) {
		inode_used_directory[i]=0;
	}
	for (int i = 0 ; i < sb->ninodes; i++) {
		//struct dinode * current_inode = inode_ptr + i;
		if (current_inode_ptr->type == T_DIR) {
		
		for (int j = 0; j < NDIRECT; j++) {
			dirent = (struct dirent*) (image_ptr + current_inode_ptr->addrs[j] *BSIZE); 		
			for (int h = 0; h < (BSIZE/sizeof(struct dirent)); h++) {
			
				if ((dirent->inum) > 0 ) {
					if ((strcmp(dirent->name, ".")!=0) && (strcmp(dirent->name, "..")!=0)){
						inode_used_directory[dirent->inum] +=1;
					}
				}
				dirent++;
			}
		}
		 
		
		uint* indirect_ptr = (uint*) (image_ptr + current_inode_ptr->addrs[NDIRECT] *BSIZE);
		for (int n = 0; n < (BSIZE/sizeof(int)); n++) {
			dirent = (struct dirent*) (image_ptr + ((uint)*indirect_ptr)*BSIZE);
			for (int k = 0 ; k < ((BSIZE/sizeof(dirent))); k++) {
				if ((dirent->inum)> 0){
					 if ((strcmp(dirent->name, ".")!=0) && (strcmp(dirent->name, "..")!=0)){
                                                inode_used_directory[dirent->inum] +=1;
                                        }
                                }
				dirent++;
			}			
		   	indirect_ptr++;
		}
		}


		current_inode_ptr++;
	}
	inode_used_directory[1] = 1;
	
}
