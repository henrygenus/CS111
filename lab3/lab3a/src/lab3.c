#include <stdio.h>      /* print() */
#include <sys/stat.h>   /* S_ISREG, S_ISDIR, S_ISLNK */
#include <unistd.h>     /* pread() */
#include <stdlib.h>     /* free(), malloc() */
#include <time.h>       /* for tm, getime() */
#include "lab3.h"

inline int print_error_msg(char *error_msg) {
    if (error_msg == NULL) perror(NULL);
    else fprintf(stderr, "%s\n", error_msg);
    return 2;
}

inline __u32 resolve_address(__u32 absolute_block_id) {
    return absolute_block_id * block_size;
}

inline __u8 is_used(int block_num, __u8 *bitmap) {
    int index = 0, offset = 0;
        if (block_num == 0)
            return 1;
    index = (block_num - 1) / 8;
    offset = (block_num - 1) % 8;
    return (bitmap[index] & (1 << offset));
}

inline void print_super(struct ext2_super_block *super) {
    fprintf(stdout, "SUPERBLOCK,%u,%u,%i,%hu,%u,%u,%u\n", super->s_blocks_count,
            super->s_inodes_count, block_size, super->s_inode_size,
            super->s_blocks_per_group, super->s_inodes_per_group, super->s_first_ino);
}

inline void print_group(struct ext2_group_desc *group, __u32 group_num,  __u32
                        blocks_in_group, __u32 inodes_in_group) {
    fprintf(stdout, "GROUP,%u,%u,%u,%u,%u,%u,%u,%u\n", group_num, blocks_in_group,
            inodes_in_group, group->bg_free_blocks_count, group->bg_free_inodes_count,
            group->bg_block_bitmap, group->bg_inode_bitmap, group->bg_inode_table);
}

void print_inode(struct ext2_inode *inode, __u32 inode_id) {
    char type, ctime[30], mtime[30], atime[30];
    time_t time; struct tm ts;
    int ctr = 0;
    
    // get inode type
    if (S_ISREG(inode->i_mode)) type = 'f';
    else if (S_ISDIR(inode->i_mode)) type = 'd';
    else if (S_ISLNK(inode->i_mode)) type = 's';
    else type = '?';
    
    // get inode dates
    time = inode->i_ctime; ts = *gmtime(&time);
    strftime(ctime, sizeof(ctime), "%m/%d/%y %H:%M:%S", &ts);
    time = inode->i_mtime; ts = *gmtime(&time);
    strftime(mtime, sizeof(mtime), "%m/%d/%y %H:%M:%S", &ts);
    time = inode->i_atime; ts = *gmtime(&time);
    strftime(atime, sizeof(atime), "%m/%d/%y %H:%M:%S", &ts);
    
    fprintf(stdout, "INODE,%u,%c,%o,%u,%u,%u,%s,%s,%s,%u,%u", inode_id, type,
            inode->i_mode & 0xFFF, inode->i_uid, inode->i_gid, inode->i_links_count,
            ctime, mtime, atime, inode->i_size, inode->i_blocks);
    if (type != 's' || inode->i_size < EXT2_N_BLOCKS*sizeof(inode->i_block))
        for ( ; ctr < EXT2_N_BLOCKS; ctr++) fprintf(stdout, ",%u", inode->i_block[ctr]);
    fprintf(stdout, "\n");
}

__u32 print_directory(struct ext2_inode *directory, int fd, int dir_inode_num) {
    __u32 offset = 0, block_num = 0, address;
    struct ext2_dir_entry dir_entry;
    do {
        do {
            if (directory->i_block[block_num] == 0) break;
            address = resolve_address(directory->i_block[block_num]) + offset % block_size;
            if (pread(fd, &dir_entry, sizeof(dir_entry), address) == -1) return(SYS_ERROR);
            if (dir_entry.inode == 0 || dir_entry.name_len == 0) break;
                fprintf(stdout, "DIRENT,%u,%u,%u,%u,%u,\'%s\'\n", dir_inode_num,
                        offset, dir_entry.inode, dir_entry.rec_len,
                        dir_entry.name_len, dir_entry.name);
            offset += dir_entry.rec_len;
            } while (offset % block_size);
        } while (++block_num < directory->i_blocks && dir_entry.inode != 0);
    return 0;
}

void print_blocks(struct ext2_inode *inode, int fd, __u32 index) {
    __u32 block_ctr, offset = EXT2_NDIR_BLOCKS, blocks_per_step = 1, ctr, lim;
    for (block_ctr = EXT2_NDIR_BLOCKS; block_ctr < EXT2_N_BLOCKS; block_ctr++) {
        if (inode->i_block[block_ctr] != 0) {
            print_block_recursive(inode, index, fd, block_ctr - EXT2_NDIR_BLOCKS + 1,
                                  &offset, inode->i_block[block_ctr]);
        }
        offset = EXT2_NDIR_BLOCKS;
        for (lim = 0; lim <= (block_ctr - EXT2_NDIR_BLOCKS); lim++) {
            blocks_per_step = (block_size/sizeof(__u32));
            for (ctr = 0; ctr < lim; ctr++) blocks_per_step *= (block_size/sizeof(__u32));
            offset += blocks_per_step;
        }
    }
}

// print children
__u32 print_block_recursive(struct ext2_inode *inode, __u32 inode_index, int fd, int depth,
                            __u32 *logical_block_offset, __u32 physical_offset) {
    if (depth == 0) return 0;
    __u32 offset = 0, *ref = (__u32*)malloc(block_size);
    if (ref == NULL) return (SYS_ERROR);
    if (pread(fd, ref, block_size, resolve_address(physical_offset)) == -1) return SYS_ERROR;
    do {
        if (ref[offset] != 0) {
            fprintf(stdout, "INDIRECT,%u,%u,%u,%u,%u\n", inode_index, depth,
                    *logical_block_offset, physical_offset, ref[offset]);
            print_block_recursive(inode, inode_index, fd, depth-1,
                                  logical_block_offset, ref[offset]);
            }
        *(logical_block_offset) += 1;
    } while((++offset) * sizeof(__u32) < block_size);
    free(ref);
    return 0;
}
