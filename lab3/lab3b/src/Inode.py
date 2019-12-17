import constants


class Inode:
    def __init__(self, link_count: int = 0, file_type: str = '?'):
        self.m_type = str(file_type)
        self.m_data_link_count = int(link_count)
        self.m_link_count = 0

    def add_link(self):
        self.m_link_count += 1

    def report_links(self, inode_number: int):
        if not self.m_link_count == self.m_data_link_count:
            print("INODE " + str(inode_number) + " HAS " + str(self.m_link_count) +
                  " LINKS BUT LINKCOUNT IS " + str(self.m_data_link_count))
            constants.return_value = 2

    # ############################################################################### #
    # ########################## RELATED UTILITY FUNCTIONS ########################## #
    # ############################################################################### #


def inode_report_free(inode_number: int):
    print("ALLOCATED INODE " + str(inode_number) + " ON FREELIST")
    constants.return_value = 2


def inode_report_unallocated(inode_number: int):
    print("UNALLOCATED INODE " + str(inode_number) + " NOT ON FREELIST")
    constants.return_value = 2


def report_wrong_link(parent, child, name, destination=None):
    if destination is None:
        destination = parent
    print("DIRECTORY INODE " + str(parent) +
          " NAME " + str(name) +
          " LINK TO INODE " + str(child) +
          " SHOULD BE " + str(destination))
    constants.return_value = 2


def report_invalid_link(parent_inode_number: int, inode_number: int, name: str):
    report_bad_link(parent_inode_number, inode_number, name, "INVALID")
    constants.return_value = 2


def report_unallocated_link(parent_inode_number: int, inode_number: int, name: str):
    report_bad_link(parent_inode_number, inode_number, name, "UNALLOCATED")
    constants.return_value = 2


def report_bad_link(parent_inode_number: int, inode_number: int, name: str, type: str):
    print("DIRECTORY INODE " + str(parent_inode_number) +
          " NAME " + name + " " + type +
          " INODE " + str(inode_number))
    constants.return_value = 2

