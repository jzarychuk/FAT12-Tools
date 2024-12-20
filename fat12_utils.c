#include "fat12_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

const int OS_NAME_START_BYTE = 3;
const int OS_NAME_LENGTH_BYTES = 8;
const int LABEL_START_BYTE = 43;
const int LABEL_LENGTH_BYTES = 11;
const int ROOT_DIR_START_SECTOR = 19;
const int ROOT_DIR_END_SECTOR = 32;
const int SECTOR_SIZE_BYTES = 512;
const int SECTOR_SIZE_ENTRIES = 16;
const int DIR_ENTRY_ATTRIBUTE_BYTE = 11;
const int TOTAL_SECTOR_COUNT_START_BYTE = 19;
const int TOTAL_SECTOR_COUNT_LENGTH_BYTES = 2;
const int FAT_START_SECTOR = 1;
const int BIT_LENGTH = 12;
const int FIRST_LOGICAL_CLUSTER_BYTE1 = 26;
const int FIRST_LOGICAL_CLUSTER_BYTE2 = 27;
const int ATTRIBUTE_VOLUME_LABEL_BIT_MASK = 0x08;
const int ATTRIBUTE_SUBDIRECTORY_BIT_MASK = 0x10;
const int FILE_SIZE_START_BYTE = 28;
const int FILE_SIZE_LENGTH_BYTES = 4;
const int SECTORS_PER_FAT_START_BYTE = 22;
const int SECTORS_PER_FAT_LENGTH_BYTES = 2;
const int NUM_FAT_COPIES_START_BYTE = 16;
const int NUM_FAT_COPIES_LENGTH_BYTES = 1;
const int FILENAME_START_BYTE = 0;
const int FILENAME_LENGTH_BYTES = 8;
const int EXTENSION_START_BYTE = 8;
const int EXTENSION_LENGTH_BYTES = 3;
const int FILE_CREATE_DATE_START_BYTE = 16;
const int FILE_CREATE_DATE_LENGTH_BYTES = 2;
const int FILE_CREATE_TIME_START_BYTE = 14;
const int FILE_CREATE_TIME_LENGTH_BYTES = 2;

/*
 * Finds a directory entry with the specified attribute in the given sector within the file.
 * The caller is responsible for freeing the returned memory.
 *
 * @param file The file pointer.
 * @param attribute The attribute to match.
 * @param sector_start_byte The starting byte of the sector.
 * @return A pointer to the matched entry or NULL if no entry is found.
 */
char* find_directory_entry (FILE* file, char attribute, long int sector_start_byte) {

        size_t entry_size_bytes = SECTOR_SIZE_BYTES / SECTOR_SIZE_ENTRIES;
        char* entry = calloc(entry_size_bytes + 1, sizeof(char));
        if (entry == NULL) {
                perror("Memory allocation failed");
                fclose(file);
                exit(EXIT_FAILURE);
        }

        // Seek to position of given sector
        if (fseek(file, sector_start_byte, SEEK_SET) != 0) { // Move file pointer sector_start_byte number of bytes relative to start of file
                perror("Error seeking to sector");
                fclose(file);
                exit(EXIT_FAILURE);
        }

        // Traverse entries in the sector
        for (int i = 0; i < SECTOR_SIZE_ENTRIES; i++) {

                // Read the next entry
                if (fread(entry, 1, entry_size_bytes, file) != entry_size_bytes) {
                        perror("Error reading entry");
                        fclose(file);
                        exit(EXIT_FAILURE);
                }

                // Check attribute byte and return entry upon match
                if (entry[DIR_ENTRY_ATTRIBUTE_BYTE] == attribute) {
                        return entry;
                }

        }

        free(entry);
        return NULL;

}

// Function to get the first logical cluster of the provided directory entry
uint16_t get_first_logical_cluster (char* entry) {

        return (uint16_t)((unsigned char)entry[FIRST_LOGICAL_CLUSTER_BYTE1] | ((unsigned char)entry[FIRST_LOGICAL_CLUSTER_BYTE2] << 8));

}

// Function to get the file size of the provided directory entry
uint32_t get_file_size (char* entry) {

        uint32_t file_size = 0;
        for (int i = 0; i < FILE_SIZE_LENGTH_BYTES; i++) {
                file_size |= (uint32_t)((unsigned char)entry[FILE_SIZE_START_BYTE + i]) << (8 * i);
        }

        return file_size;

}

// Function to read a field of data from the provided directory entry in the provided disk image
char* read_directory_entry_data (FILE* file, char* entry, int start_byte, int length_bytes) {

        // Allocate memory for string
        char* data = (char*)calloc(length_bytes + 1, sizeof(char));
        if (data == NULL) {
                perror("Memory allocation failed");
                fclose(file);
                exit(EXIT_FAILURE);
        }

	// Copy the relevant bytes from the entry into data
        for (int i = 0; i < length_bytes; i++) {
                data[i] = entry[start_byte + i];
        }

	return data;

}
