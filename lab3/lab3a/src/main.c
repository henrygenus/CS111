#include <stdio.h>      /* print, stderr*/
#include <stdlib.h>     /* free(), malloc(), exit() */
#include <sys/stat.h>   /* S_ISDIR */
#include <fcntl.h>    /* open() */
#include <unistd.h>     /* pread() */
#include <math.h>       /* ceil() */
#include "lab3.h"

__u32 block_size;

int main(int argc, const char * argv[]) {
    // check command line
    if (argc != 2) {
        fprintf(stderr, "USAGE: lab3a FILE\n");
        exit(1);
    }
    // utility variables
    int fd = 0;
    __u32 group_count, group_count_check, group_ctr = 0, ctr;
    
    // ext2 structs
    struct ext2_super_block super;
    struct ext2_group_desc group;
    struct ext2_inode inode;

    // group level variables
    __u32  group_desc_address, address, blocks_in_group, inodes_in_group;
    __u8 *block_bitmap, *inode_bitmap;
    
    // open filesystem
    if ((fd = open(argv[1], O_RDONLY)) == -1){
        perror(argv[1]);
        exit(1);
    }
    
    // get super block
    if (pread(fd, &super, sizeof(super), EXT2_MIN_BLOCK_SIZE) != sizeof(super)) exit(NO_SUPER);

    // set block info constants
    block_size = EXT2_MIN_BLOCK_SIZE << super.s_log_block_size;
    group_desc_address = block_size * (super.s_first_data_block + 1);
    
    // get room to grab bitmaps
    if ((block_bitmap = (__u8*)malloc(block_size)) == NULL) exit(SYS_ERROR);
    if ((inode_bitmap = (__u8*)malloc(block_size)) == NULL) exit(SYS_ERROR);
   
    // count and check the number of groups
    group_count = ceil((double)super.s_blocks_count / super.s_blocks_count);
    group_count_check = ceil((double)super.s_inodes_count / super.s_inodes_count);
    if (group_count != group_count_check) exit(BAD_SUPER_BLOCK_DATA);
    else print_super(&super);
    
    // iterate through block groups
    while (group_ctr < group_count) {
        
        // attempt to read group descriptor
        address = group_desc_address + group_ctr * sizeof(group);
        if (pread(fd, &group, sizeof(group), address) == -1) exit(SYS_ERROR);
        
        // print group data
        blocks_in_group = super.s_blocks_count <= super.s_blocks_per_group ? super.s_blocks_count : super.s_blocks_per_group;
        inodes_in_group = super.s_inodes_count <= super.s_inodes_per_group ?
            super.s_inodes_count : super.s_inodes_per_group;
        print_group(&group, group_ctr++, blocks_in_group, inodes_in_group);
        
        // read in block and inode tables
        address = resolve_address(group.bg_block_bitmap);
        if (pread(fd, block_bitmap, block_size, address) == -1) exit(SYS_ERROR);
        address = resolve_address(group.bg_inode_bitmap);
        if (pread(fd, inode_bitmap, block_size, address) == -1) exit(SYS_ERROR);
        
        // parse block bitmap, block 1 is super
        for (ctr = 1; ctr < blocks_in_group; ctr++) {
            if (!is_used(ctr, block_bitmap)) {
                fprintf(stdout, "BFREE,%d\n", ctr);
                if (super.s_free_blocks_count-- == 0) exit(BAD_FREE_BLOCK_COUNT);
            }
        }

        // parse inode bitmap, first 10 are reserved
        for (ctr = 1; ctr <= inodes_in_group; ctr++) {
            if (!is_used(ctr, inode_bitmap)) {
                fprintf(stdout, "IFREE,%d\n", ctr);
                if (super.s_free_inodes_count-- == 0) exit(BAD_FREE_INODE_COUNT);
            }
            else {
                // get inode
                address = 
                    resolve_address(group.bg_inode_table)+ sizeof(inode)*(ctr-1);
                if (pread(fd, &inode, sizeof(inode), address) == -1) exit(SYS_ERROR);
                if (inode.i_mode == 0 || inode.i_links_count == 0) continue;
                print_inode(&inode, ctr);
                
                // print file or directory info
                if (S_ISDIR(inode.i_mode)) print_directory(&inode, fd, ctr);
                else print_blocks(&inode, fd, ctr);
            }
        }
        
        // check is not necessary since blocks in group and inodes in group < super.
        super.s_blocks_count -= blocks_in_group;
        super.s_inodes_count -= inodes_in_group;
    }
    
    // if we have seen all the blocks and empty blocks we are okay...all true = 1
    if (super.s_blocks_count || super.s_free_blocks_count ||
        super.s_inodes_count || super.s_free_inodes_count)
            exit(SUPER_AND_DATA_INCONGRUENT);
    exit(0);
}
