#ifndef FAT12_UTILS_H
#define FAT12_UTILS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

extern const int OS_NAME_START_BYTE;
extern const int OS_NAME_LENGTH_BYTES;
extern const int LABEL_START_BYTE;
extern const int LABEL_LENGTH_BYTES;
extern const int ROOT_DIR_START_SECTOR;
extern const int ROOT_DIR_END_SECTOR;
extern const int SECTOR_SIZE_BYTES;
extern const int SECTOR_SIZE_ENTRIES;
extern const int DIR_ENTRY_ATTRIBUTE_BYTE;
extern const int TOTAL_SECTOR_COUNT_START_BYTE;
extern const int TOTAL_SECTOR_COUNT_LENGTH_BYTES;
extern const int FAT_START_SECTOR;
extern const int BIT_LENGTH;
extern const int FIRST_LOGICAL_CLUSTER_BYTE1;
extern const int FIRST_LOGICAL_CLUSTER_BYTE2;
extern const int ATTRIBUTE_VOLUME_LABEL_BIT_MASK;
extern const int ATTRIBUTE_SUBDIRECTORY_BIT_MASK;
extern const int FILE_SIZE_START_BYTE;
extern const int FILE_SIZE_LENGTH_BYTES;
extern const int SECTORS_PER_FAT_START_BYTE;
extern const int SECTORS_PER_FAT_LENGTH_BYTES;
extern const int NUM_FAT_COPIES_START_BYTE;
extern const int NUM_FAT_COPIES_LENGTH_BYTES;
extern const int FILENAME_START_BYTE;
extern const int FILENAME_LENGTH_BYTES;
extern const int EXTENSION_START_BYTE;
extern const int EXTENSION_LENGTH_BYTES;
extern const int FILE_CREATE_DATE_START_BYTE;
extern const int FILE_CREATE_DATE_LENGTH_BYTES;
extern const int FILE_CREATE_TIME_START_BYTE;
extern const int FILE_CREATE_TIME_LENGTH_BYTES;

char* find_directory_entry (FILE* file, char attribute, long int sector_start_byte);
uint16_t get_first_logical_cluster (char* entry);
uint32_t get_file_size (char* entry);
char* read_directory_entry_data (FILE* file, char* entry, int start_byte, int length_bytes);

#endif
