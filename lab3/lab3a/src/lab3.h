#include "ext2_fs.h"

#ifndef lab3_h
#define lab3_h

// error messages
#define SYS_ERROR print_error_msg(NULL)
#define SUPER_AND_DATA_INCONGRUENT print_error_msg(SUPER_AND_DATA_INCONGRUENT_MSG)
#define BAD_FREE_BLOCK_COUNT print_error_msg(BAD_FREE_BLOCK_COUNT_MSG)
#define BAD_FREE_INODE_COUNT print_error_msg(BAD_FREE_INODE_COUNT_MSG)
#define BAD_BLOCK_COUNT print_error_msg(BAD_BLOCK_COUNT_MSG)
#define BAD_INODE_COUNT print_error_msg(BAD_INODE_COUNT_MSG)
#define BAD_SUPER_BLOCK_DATA print_error_msg(BAD_SUPER_BLOCK_DATA_MSG)
#define NO_SUPER print_error_msg(NO_SUPER_MSG)
#define SUPER_AND_DATA_INCONGRUENT_MSG "File system data does not match data in super block"
#define BAD_FREE_BLOCK_COUNT_MSG "Free block count does not match data in super block"
#define BAD_FREE_INODE_COUNT_MSG "Free inode count does not match data in super block"
#define BAD_BLOCK_COUNT_MSG "Block count does not match data in super block"
#define BAD_INODE_COUNT_MSG "Inode count does not match data in super block"
#define BAD_SUPER_BLOCK_DATA_MSG "Bad super block data"
#define NO_SUPER_MSG "Could not read super block data"

// extern so that it need not be passed
extern __u32 block_size;

/*
* utility functions
*/

// for use with above constants
int print_error_msg(char *error_msg);

// get the block address from the index
__u32 resolve_address(__u32 absolute_block_id);

// test if a given block in a bitmap is free
__u8 is_used(int block_num, __u8 *bitmap);


/*
 * ext2 structure printing functions
 */

// print the information required for a super block
void print_super(struct ext2_super_block *super);

// print the information required for a group
void print_group(struct ext2_group_desc *group, __u32 group_num,  __u32 blocks_in_group, __u32 inodes_in_group);

// print the information required for a pure inode
void print_inode(struct ext2_inode *inode, __u32 inode_id);

// print all directory entries recursively
__u32 print_directory(struct ext2_inode *directory, int fd, int dir_inode_num);

// print info for all blocks of an inode (recursive)
void print_blocks(struct ext2_inode *inode, int fd, __u32 index);

// print a given block recursively
__u32 print_block_recursive(struct ext2_inode *inode, __u32 inode_index, int fd, int depth, __u32 *logical_block_offset, __u32 physical_offset);

#endif /* lab3_h */
