#!/usr/bin/python3

# #!/usr/local/cs/bin/python3.7

import sys
from Inode import *
from Block import *
from constants import *


def main():
    # info capture objects
    data = {}
    file = []
    filename = []
    super_block = None
    for key in entry_types:
        data[key] = []

    # OPEN FILE SYSTEM
    try:
        if sys.argv[1] == "-":
            raise IndexError
        if len(sys.argv) > 2:
            print("Extra operands" + str(sys.argv[2:]))
            exit(1)
        filename = sys.argv[1]
        file = open(filename, 'r')
    except IndexError:
        if len(sys.argv) > 1:
            print("Extra operands" + str(sys.argv[1:]))
            exit(1)
        filename = "stdin"
        file = sys.stdin
    except OSError:
        if len(sys.argv) != 1:
            print("Bad options" + str(sys.argv[1:]), file=sys.stderr)
            exit(1)

    # ACQUIRE FILE SYSTEM
    try:
        # we treat the super block differently since there is only one
        while True:
            line = file.readline()
            if not line:
                break
            words = list(filter(None, line.strip().split(",")))
            if words[0] == "SUPERBLOCK":
                super_block = words
            elif line is not "":
                data[words[0]].append(words)
        file.close()
    except IOError:
        print("Could not read" + filename, file=sys.stderr)
        exit(1)
    except EOFError:
        try:
            assert (len(data) is not None)
        except AssertionError:
            print("Could not read data", file=sys.stderr)
            exit(1)
        try:
            assert (super_block is not None)
        except AssertionError:
            print("No super block found", file=sys.stderr)
            exit(1)

    # set useful constants
    try:
        num_blocks = int(super_block[NUM_BLOCKS_INDEX])
        block_size = int(super_block[BLOCK_SIZE_INDEX])
    except IndexError or TypeError:
        print("Corrupt data block")
        exit(1)

    # FILTER FILE SYSTEM DATA
    blocks = {}
    inodes = {}
    entries = {}
    block_state = [VALID] * (int(num_blocks) + 1)
    inode_state = [VALID] * (int(num_blocks) + 1)

    # ############################################################################### #
    # ########################### GET DATA FOR FILE SYSTEM ########################## #
    # ############################################################################### #

    # mark boot block as reserved
    for block in range(int(block_size/1024)):
        block_state[block] = RESERVED

    # read in all free blocks
    for block in data["BFREE"]:
        block_state[int(block[FREE_BLOCK_INDEX])] = FREE

    # read in all free inodes
    for inode in data["IFREE"]:
        inode_state[int(inode[FREE_INODE_INDEX])] = FREE

    # read in reserved blocks from group
    for group in data["GROUP"]:
        group_first_block = int(group[GROUP_NUMBER_INDEX]) * \
                      int(super_block[BLOCKS_PER_GROUP_INDEX]) +\
                      int(block_size / 1024)
        group_last_metadata_block = group_first_block + int(group[GROUP_FIRST_INODE_INDEX]) + 1

        # account for blocks which store metadata
        for block in range(group_first_block, group_last_metadata_block + 1):
            block_state[block] = RESERVED

    # read in all allocated inodes
    for inode in data["INODE"]:
        # need not check for duplicate/invalid inodes according to spec
        # check for free
        inode_number = int(inode[INODE_SELF_INDEX])
        entries[inode_number] = []
        if inode_state[inode_number] is FREE:
            inode_report_free(inode_number)
        inodes[inode_number] = \
            Inode(int(inode[INODE_LINK_COUNT_INDEX]), inode[INODE_FILE_TYPE])

        # check for symbolic links
        if int(inode[INODE_SIZE_INDEX]) < 60:
            continue

        # read in blocks which are part of inode
        # check for invalid, free, reserved
        for logical_block in range(FIRST_BLOCK, LAST_BLOCK + 1):
            absolute_block = int(inode[logical_block])
            if not absolute_block == 0:
                offset = int(find_offset_for_inode_block(logical_block, block_size))
                depth = int(logical_block - LAST_DIRECT_BLOCK)
                block_context = (inode_number, offset, depth)
                if check_and_report_validity(absolute_block, block_context, num_blocks) and \
                        not check_and_report_block(absolute_block, block_context, block_state[absolute_block]):
                    try:
                        blocks[absolute_block].add(inode_number, offset, depth)
                    except KeyError:
                        blocks[absolute_block] = Block(absolute_block, inode_number, offset, depth)

    # read in indirect blocks
    for block in data["INDIRECT"]:
        # check for invalid, free, reserved
        block_number = int(block[INDIRECT_BLOCK_INDEX])
        parent_inode = int(block[INDIRECT_PARENT_INODE_INDEX])
        offset = int(block[INDIRECT_OFFSET_INDEX])
        depth = int(block[INDIRECT_DEPTH_INDEX])
        block_context = (parent_inode, offset, depth)

        if check_and_report_validity(block_number, block_context, num_blocks) and \
                not check_and_report_block(block_number, block_context, block_state[block_number]):
            try:
                blocks[block_number].add(parent_inode, 0, 0)
            except KeyError:
                blocks[block_number] = Block(block_number, parent_inode, offset, depth)

    # read in directory entries
    for dirent in data["DIRENT"]:
        try:
            inodes[int(dirent[DIRENT_SELF_INDEX])].add_link()
            entries[int(dirent[DIRENT_PARENT_INDEX])]. \
                append((int(dirent[DIRENT_SELF_INDEX]), dirent[DIRENT_NAME_INDEX]))
        except KeyError:
            parent_inode = int(dirent[DIRENT_PARENT_INDEX])
            child_inode = int(dirent[DIRENT_SELF_INDEX])
            name = dirent[DIRENT_NAME_INDEX]
            if not 1 <= child_inode <= int(super_block[NUM_INODES_INDEX]):
                report_invalid_link(parent_inode, child_inode, name)
            else:
                report_unallocated_link(parent_inode, child_inode, name)

    # ############################################################################### #
    # ######################### CHECK DATA FROM FILE SYSTEM ######################### #
    # ############################################################################### #

    # report on block w/ reserved = true if status
    for block_num in range(int(max(blocks.keys())) + 1):
        try:
            if block_state[block_num] is VALID:
                blocks[block_num].report_check_duplicates()
        except (IndexError, KeyError):
            block_report_unreferenced(str(block_num))

    # root inode is a corner case not within the range of file inodes
    try:
        inodes[ROOT].report_links(ROOT)
    except KeyError:
        inode_report_unallocated(ROOT)

    # check all inodes within range to report if they are not free
    for inode_number in range(int(super_block[FIRST_INODE_INDEX]),
                              int(super_block[NUM_INODES_INDEX])):
        if inode_state[inode_number] is not FREE:
            try:
                inodes[inode_number].report_links(inode_number)
            except KeyError:
                inode_report_unallocated(inode_number)

    # directory tree represented in dictionary of form dictionary[parent] = (child, name)
    for trip, list_child_name in entries.items():
        for child_name in list_child_name:
            # verify entry '.' points to self
            if child_name[NAME] == "\'.\'":
                if trip != child_name[CHILD]:
                    report_wrong_link(trip, child_name[CHILD], child_name[NAME])
            # for each non '..' entry, verify it has a '..' that points to the parent
            elif not child_name[NAME] == "\'..\'":
                if inodes[child_name[CHILD]].m_type == DIRECTORY \
                        and (trip, "'..'") not in entries[child_name[CHILD]]:
                    report_wrong_link(child_name[CHILD], trip, "..", trip)
            # ensure ROOT's children point to it
            if trip == ROOT and child_name[NAME] == "\'..\'" and child_name[CHILD] != ROOT:
                report_wrong_link(ROOT, child_name[CHILD], "'..'", ROOT)

    exit(constants.return_value)


if __name__ == "__main__":
    main()
