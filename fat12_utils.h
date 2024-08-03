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
extern const int SECTORS_PER_FAT_START_BYTE;
extern const int SECTORS_PER_FAT_LENGTH_BYTES;
extern const int TOTAL_SECTOR_COUNT_START_BYTE;
extern const int TOTAL_SECTOR_COUNT_LENGTH_BYTES;
extern const int FAT_START_SECTOR;
extern const int BIT_LENGTH;
extern const int NUM_FAT_COPIES_START_BYTE;
extern const int NUM_FAT_COPIES_LENGTH_BYTES;

char* find_directory_entry (FILE* file, char attribute, long int sector_start_byte);

#endif
