#include "fat12_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>
#include <ctype.h>

// Function to trim trailing spaces from a string
void trim_trailing_spaces(char* str) {
	int length = strlen(str);
	while (length > 0 && isspace((unsigned char)str[length - 1])) {
		str[length - 1] = '\0';
		length--;
	}
}

// Function to format the creation date and time of a directory entry in the provided disk image
char* format_creation_datetime(FILE* file, char* creation_date, char* creation_time){	

	// Deserialize data
	uint16_t date = (unsigned char)creation_date[0] | (unsigned char)creation_date[1] << 8;
	uint16_t time = (unsigned char)creation_time[0] | (unsigned char)creation_time[1] << 8;

	// Allocate memory for string
        char* formatted_datetime = (char*)calloc(17, sizeof(char));
        if (formatted_datetime == NULL) {
                perror("Memory allocation failed");
                fclose(file);
                exit(EXIT_FAILURE);
        }

	// Extract the date and time from raw data
	int year, month, day, hour, minute;
	year = ((date & 0xFE00) >> 9) + 1980; // Year is stored in the high seven bits as a value since 1980
	month = (date & 0x1E0) >> 5; // Month is stored in the middle four bits
	day = (date & 0x1F); // Day is stored in the low five bits
	hour = (time & 0xF800) >> 11; // Hour is stored in the high five bits
	minute = (time & 0x7E0) >> 5; // Minute is stored in the middle six bits

	// Format the date and time into human-readable string
	snprintf(formatted_datetime, 17, "%04d-%02d-%02d %02d:%02d", year, month, day, hour, minute);

	return formatted_datetime; // Caller is responsible for freeing this memory

}

// Function to print all files, organized by directory, in the provided disk image
void print_files (FILE* file, long int dir_start_byte, int dir_length_sectors) {

        // Seek to position of directory
        if (fseek(file, dir_start_byte, SEEK_SET) != 0) { // Move file pointer dir_start_byte number of bytes relative to start of file
                perror("Error seeking to directory");
                fclose(file);
                exit(EXIT_FAILURE);
        }

        // Allocate memory for binary data
        size_t entry_size_bytes = SECTOR_SIZE_BYTES / SECTOR_SIZE_ENTRIES;
        char* entry = calloc(entry_size_bytes, sizeof(char));
        if (entry == NULL) {
                perror("Memory allocation failed");
                fclose(file);
                exit(EXIT_FAILURE);
        }

	// Read all entries in directory
        for (int sector_offset = 0; sector_offset < dir_length_sectors; sector_offset++) { // Traverse all sectors of directory
                for (int i = 0; i < SECTOR_SIZE_ENTRIES; i++) { // Traverse all entries of sector

                        // Read the entry
                        if (fread(entry, 1, 32, file) != 32) {
                                perror("Error reading entry");
                                fclose(file);
                                exit(EXIT_FAILURE);
                        }

                        // Skip the entry if first logical cluster is 0 or 1
                        uint16_t first_logical_cluster = get_first_logical_cluster(entry);
                        if (first_logical_cluster == 0 || first_logical_cluster == 1) {
                                continue;
                        }

                        // Skip the entry if attribute is 0x0F (indicating entry is part of a long file name)
                        if (entry[DIR_ENTRY_ATTRIBUTE_BYTE] == 0x0F) {
                                continue;
                        }

                        // Skip the entry if first byte is 0xE5 (indicating entry is free)
                        if (entry[0] == 0xE5) {
                                continue;
                        }

                        // Skip the entry if volume label bit of attribute is set
                        if (entry[DIR_ENTRY_ATTRIBUTE_BYTE] & ATTRIBUTE_VOLUME_LABEL_BIT_MASK) {
                                continue;
                        }

                        // If the entry is a subdirectory, print it, traverse it, then proceed to next entry
                        if (entry[DIR_ENTRY_ATTRIBUTE_BYTE] & ATTRIBUTE_SUBDIRECTORY_BIT_MASK) {
				char* filename = read_directory_entry_data(file, entry, FILENAME_START_BYTE, FILENAME_LENGTH_BYTES);
				fprintf(stdout, "%s\n--------------------------------------------------\n", filename);
				free(filename);
                                long int subdir_start_byte = (33 + first_logical_cluster - 2) * SECTOR_SIZE_BYTES;
                                uint32_t subdir_file_size = get_file_size(entry);
                                int subdir_length_sectors = (subdir_file_size + SECTOR_SIZE_BYTES - 1) / SECTOR_SIZE_BYTES;
                                print_files(file, subdir_start_byte, subdir_length_sectors);
                                continue;
                        }

                        // Print the entry as a regular file
			char* file_size_data = read_directory_entry_data(file, entry, FILE_SIZE_START_BYTE, FILE_SIZE_LENGTH_BYTES);
			uint32_t file_size = 
				(unsigned char)file_size_data[0] | 
				(unsigned char)file_size_data[1] << 8 | 
				(unsigned char)file_size_data[2] << 16 | 
				(unsigned char)file_size_data[3] << 24;
			free(file_size_data);
			char* filename = read_directory_entry_data(file, entry, FILENAME_START_BYTE, FILENAME_LENGTH_BYTES);
			char* extension = read_directory_entry_data(file, entry, EXTENSION_START_BYTE, EXTENSION_LENGTH_BYTES);
			trim_trailing_spaces(filename);
			trim_trailing_spaces(extension);
			char* filename_extension = (char*)calloc(FILENAME_LENGTH_BYTES + EXTENSION_LENGTH_BYTES + 2, sizeof(char)); // +2 for the dot and null terminator
			if (filename_extension == NULL) {
				perror("Memory allocation failed");
                		fclose(file);
                		exit(EXIT_FAILURE);
			}
			snprintf(filename_extension, FILENAME_LENGTH_BYTES + EXTENSION_LENGTH_BYTES + 2, "%s.%s", filename, extension);
			free(extension);
                        free(filename);
			char* creation_date_data = read_directory_entry_data(file, entry, FILE_CREATE_DATE_START_BYTE, FILE_CREATE_DATE_LENGTH_BYTES);
			char* creation_time_data = read_directory_entry_data(file, entry, FILE_CREATE_TIME_START_BYTE, FILE_CREATE_TIME_LENGTH_BYTES);
			char* creation_datetime = format_creation_datetime(file, creation_date_data, creation_time_data);
			free(creation_date_data);
			free(creation_time_data);
			fprintf(stdout, "F %-10u %-*s %s\n", file_size, FILENAME_LENGTH_BYTES + EXTENSION_LENGTH_BYTES + 1, filename_extension, creation_datetime); // +1 for the dot
			free(creation_datetime);
			free(filename_extension);

                }
        }

        free(entry);

}

int main (int argc, char* argv[]) {

        if (argc < 2) {
                fprintf(stderr, "Usage: %s <disk.IMA>\n", argv[0]);
                exit(2);
        }

        // Open provided file
        FILE* file = fopen(argv[1], "r");
        if (file == NULL) {
                perror("Error opening file");
                exit(EXIT_FAILURE);
        }

	// Print all the files
	long int root_dir_start_byte = ROOT_DIR_START_SECTOR * SECTOR_SIZE_BYTES;
        int root_dir_length_sectors = ROOT_DIR_END_SECTOR - ROOT_DIR_START_SECTOR + 1;
	fprintf(stdout, "Root\n--------------------------------------------------\n");
        print_files(file, root_dir_start_byte, root_dir_length_sectors);

	fclose(file);
	return 0;

}
