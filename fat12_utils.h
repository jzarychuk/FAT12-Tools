#ifndef FAT12_UTILS_H
#define FAT12_UTILS_H

#include <stdio.h>
#include <stdlib.h>

extern const int OS_NAME_START_BYTE;
extern const int OS_NAME_LENGTH_BYTES;
extern const int LABEL_START_BYTE;
extern const int LABEL_LENGTH_BYTES;
extern const int ROOT_DIR_START_SECTOR;
extern const int ROOT_DIR_END_SECTOR;
extern const int SECTOR_SIZE_BYTES;
extern const int SECTOR_SIZE_ENTRIES;
extern const int ATTRIBUTE_BYTE;

char* find_entry (FILE* file, char attribute, long int sector_start_byte);

#endif
