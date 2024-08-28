#include "fat12_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

// Function to read a field of data from the boot sector of the provided disk image
char* read_boot_sector_data(FILE* file, int start_byte, int length_bytes) {

        // Seek to position of the field in boot sector
        if (fseek(file, start_byte, SEEK_SET) != 0) { // Move file pointer start_byte number of bytes relative to start of file
                perror("Error seeking to the field in boot sector");
                fclose(file);
                exit(EXIT_FAILURE);
        }

        // Allocate memory for string (while it may be for raw data, everything will be null-terminated for simplicity and safety)
        char* data = (char*)calloc(length_bytes + 1, sizeof(char));
        if (data == NULL) {
                perror("Memory allocation failed");
                fclose(file);
                exit(EXIT_FAILURE);
        }

        // Read the field from boot sector
        if (fread(data, 1, length_bytes, file) != length_bytes) { // Read one byte at a time into data for length_bytes number of times
                perror("Error reading field from boot sector");
                fclose(file);
                exit(EXIT_FAILURE);
        }

        return data;

}

// Function to check whether the char array for the label has changed from when it was initialized to all zeros
int is_label_changed (char* label) {

	for (int i = 0; i < LABEL_LENGTH_BYTES; i++) {
		if (label[i] != ' ') {
			return 1; // True, label is changed
		}
	}
	return 0; // False, label is unchanged

}

// Function to get the label of the provided disk image
char* get_label(FILE* file) {

	char* label = read_boot_sector_data(file, LABEL_START_BYTE, LABEL_LENGTH_BYTES);

	// If label not found, then check root directory
        if (!is_label_changed(label)) {

                long int root_dir_start_byte = ROOT_DIR_START_SECTOR * SECTOR_SIZE_BYTES;
                int root_dir_length_bytes = (ROOT_DIR_END_SECTOR - ROOT_DIR_START_SECTOR + 1) * SECTOR_SIZE_BYTES;
                char* entry;

                // Iterate through each sector in root directory
                for (int sector_offset = 0; sector_offset < root_dir_length_bytes; sector_offset += SECTOR_SIZE_BYTES) {
                        entry = find_directory_entry(file, 0x08, root_dir_start_byte + sector_offset);
                        if (entry != NULL) {
                                strncpy(label, entry, 11); // Copy the first 11 characters of the entry into the label
                                free(entry);
                                break;
                        }
                }

        }		

	return label;

}

// Function to get the unused sector count of the provided disk image
int get_unused_sector_count(FILE* file) {

	char* total_sector_count_data = read_boot_sector_data(file, TOTAL_SECTOR_COUNT_START_BYTE, TOTAL_SECTOR_COUNT_LENGTH_BYTES);
        uint16_t total_sector_count = ((unsigned char)total_sector_count_data[0]) | (unsigned char)total_sector_count_data[1] << 8;
        free(total_sector_count_data);
	int unused_sector_count = 0;

        // Seek to position of first FAT table
        int fat1_start_byte = FAT_START_SECTOR * SECTOR_SIZE_BYTES;
        if (fseek(file, fat1_start_byte, SEEK_SET) != 0) { // Move file pointer fat1_start_byte number of bytes relative to start of file
                perror("Error seeking to first FAT table");
                fclose(file);
                exit(EXIT_FAILURE);
        }

	// Allocate memory for binary data
        size_t num_bytes = BIT_LENGTH * 2 / 8;
        char* entries = calloc(num_bytes, sizeof(char));
        if (entries == NULL) {
                perror("Memory allocation failed");
                fclose(file);
                exit(EXIT_FAILURE);
        }

	// Iterate through fat entries in pairs, where each entry maps to a physical sector within the total sector count range (skipping the first two entries, since they are reserved)
        for (int fat_entry_number = 2; (33 + fat_entry_number - 2) < total_sector_count; fat_entry_number += 2) {

		// Read one byte at a time into entries for num_bytes number of times
                if (fread(entries, 1, num_bytes, file) != num_bytes) {
                        perror("Error reading reading entries");
                        fclose(file);
                        exit(EXIT_FAILURE);
                }

                uint16_t entry1 = (entries[0] << 4) | (entries[1] >> 4); // Extract the first 12 bits
                uint16_t entry2 = ((entries[1] & 0x0F) << 8) | entries[2]; // Extract the second 12 bits

                if ((entry1 & 0x0FFF) == 0x000) { // Compare only the lower 12 bits of entry1 to 0x000
                        unused_sector_count++;
                }
                if ((entry2 & 0x0FFF) == 0x000) { // Compare only the lower 12 bits of entry2 to 0x000
                        unused_sector_count++;
                }

        }

	free(entries);
	return unused_sector_count;

}

// Function to get the number of files in the provided disk image, starting from the specified byte, and spanning the specified number of sectors
int get_num_files(FILE* file, long int dir_start_byte, int dir_length_sectors) {

	int num_files = 0;

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

			// If the entry is a subdirectory, traverse it first, then skip it
			if (entry[DIR_ENTRY_ATTRIBUTE_BYTE] & ATTRIBUTE_SUBDIRECTORY_BIT_MASK) {
				long int subdir_start_byte = (33 + first_logical_cluster - 2) * SECTOR_SIZE_BYTES;
				uint32_t subdir_file_size = get_file_size(entry);
				int subdir_length_sectors = (subdir_file_size + SECTOR_SIZE_BYTES - 1) / SECTOR_SIZE_BYTES;
				get_num_files(file, subdir_start_byte, subdir_length_sectors);
				continue;
			}

			num_files++;

		}
	}

	free(entry);
	return num_files;

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

	// Get the OS name
	char* os_name = read_boot_sector_data(file, OS_NAME_START_BYTE, OS_NAME_LENGTH_BYTES);

	// Get the label
	char* label = get_label(file);

	// Calculate the total size
	char* total_sector_count_data = read_boot_sector_data(file, TOTAL_SECTOR_COUNT_START_BYTE, TOTAL_SECTOR_COUNT_LENGTH_BYTES);
        uint16_t total_sector_count = ((unsigned char)total_sector_count_data[0]) | (unsigned char)total_sector_count_data[1] << 8;
	free(total_sector_count_data);
	float total_size = total_sector_count * SECTOR_SIZE_BYTES;

        // Calculate the free size
	float free_size = get_unused_sector_count(file) * SECTOR_SIZE_BYTES;

	// Calculate the number of files
	long int root_dir_start_byte = ROOT_DIR_START_SECTOR * SECTOR_SIZE_BYTES;
	int root_dir_length_sectors = ROOT_DIR_END_SECTOR - ROOT_DIR_START_SECTOR + 1;
	int num_files = get_num_files(file, root_dir_start_byte, root_dir_length_sectors);
	
        // Get the number of sectors per FAT
        char* sectors_per_fat_data = read_boot_sector_data(file, SECTORS_PER_FAT_START_BYTE, SECTORS_PER_FAT_LENGTH_BYTES);
	uint16_t sectors_per_fat = ((unsigned char)sectors_per_fat_data[0]) | (unsigned char)sectors_per_fat_data[1] << 8;
	free(sectors_per_fat_data);

	// Get the number of FAT copies
	char* num_fat_copies_data = read_boot_sector_data(file, NUM_FAT_COPIES_START_BYTE, NUM_FAT_COPIES_LENGTH_BYTES);
	uint8_t num_fat_copies = (unsigned char)num_fat_copies_data[0];
	free(num_fat_copies_data);

	fprintf(stdout, "OS Name: %s\n", os_name);
	fprintf(stdout, "Label of the disk: %s\n", label);
	fprintf(stdout, "Total size of the disk: %.0f\n", total_size);
	fprintf(stdout, "Free size of the disk: %.0f\n", free_size);
	fprintf(stdout, "Number of files in the disk: %d\n", num_files);
	fprintf(stdout, "Number of sectors per FAT: %" PRIu16 "\n", sectors_per_fat);
	fprintf(stdout, "Number of FAT copies: %" PRIu8 "\n", num_fat_copies);

	free(label);
	free(os_name);
	fclose(file);
	return 0;

}
