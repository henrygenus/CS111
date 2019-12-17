import constants
from constants import RESERVED, FREE, FIRST_BLOCK, LAST_BLOCK, LAST_DIRECT_BLOCK, SIZE_OF_INDIRECT_ENTRY


class Block:
    def __init__(self, block_num: int, parent_inode: int = -1, offset: int = -1, depth: int = -1):
        self.m_block_num = int(block_num)
        self.m_links = [(int(parent_inode), int(offset), int(depth))]

    def add(self, parent_inode: int = -1, offset: int = -1, depth: int = -1):
        self.m_links.append((int(parent_inode), int(offset), int(depth)))

    def report_check_duplicates(self):
        if len(self.m_links) > 1:
            s = set()
            for context in self.m_links:
                s.add(context[0])
            if len(s) > 1:
                self.block_report_duplicate()

    def block_report_duplicate(self):
        for status in self.m_links:
            block_report_info(self.m_block_num, status[0], status[1], status[2])
        constants.return_value = 2

    # ############################################################################### #
    # ########################## RELATED UTILITY FUNCTIONS ########################## #
    # ############################################################################### #


def check_and_report_validity(absolute_block, block_context, num_blocks,):
    inode_number, offset, depth = block_context
    if not 1 <= absolute_block <= num_blocks:
        block_report_invalid(absolute_block, inode_number, offset, depth)
        return False
    return True


def check_and_report_block(absolute_block, block_context, block_state):
    inode_number, offset, depth = block_context
    if block_state is RESERVED:
        block_report_reserved(absolute_block, inode_number, offset, depth)
    elif block_state is FREE:
        block_report_free(absolute_block)
    else:
        return False
    return True


def block_report_info(block_number: int, inode: int, offset: int, depth: int, mode="DUPLICATE "):
    if int(depth) == 3:
        word = "TRIPLE INDIRECT "
    elif int(depth) == 2:
        word = "DOUBLE INDIRECT "
    elif int(depth) == 1:
        word = "INDIRECT "
    else:
        word = ""
    print(mode + word + "BLOCK " + str(block_number) +
          " IN INODE " + str(inode) +
          " AT OFFSET " + str(offset))


def block_report_invalid(block_number, inode: int, offset: int, depth: int):
    block_report_info(block_number, inode, offset, depth, "INVALID ")
    constants.return_value = 2


def block_report_reserved(block_number, inode: int, offset: int, depth: int):
    block_report_info(block_number, inode, offset, depth, "RESERVED ")
    constants.return_value = 2


def block_report_free(block_number: int):
    print("ALLOCATED BLOCK " + str(block_number) + " ON FREELIST")
    constants.return_value = 2


def block_report_unreferenced(block_number: str):
    print("UNREFERENCED BLOCK " + str(block_number))
    constants.return_value = 2


def find_offset_for_inode_block(offset, block_size):
    num_direct_blocks = offset - FIRST_BLOCK \
        if offset <= LAST_DIRECT_BLOCK + 1 else LAST_DIRECT_BLOCK - FIRST_BLOCK + 1
    num_indirect_blocks = block_size/SIZE_OF_INDIRECT_ENTRY \
        if offset > LAST_DIRECT_BLOCK + 1 else 0
    num_double_indirect_blocks = block_size/SIZE_OF_INDIRECT_ENTRY * block_size/SIZE_OF_INDIRECT_ENTRY \
        if offset == LAST_BLOCK else 0
    return num_direct_blocks + num_indirect_blocks + num_double_indirect_blocks
