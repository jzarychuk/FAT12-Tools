#include "fat12_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

// Function to get the OS name of the provided disk image
char* get_os_name(FILE* file) {

        // Seek to position of OS name in boot sector
        if (fseek(file, OS_NAME_START_BYTE, SEEK_SET) != 0) { // Move file pointer OS_NAME_START_BYTE number of bytes relative to start of file
                perror("Error seeking to OS name position of boot sector");
                fclose(file);
                exit(EXIT_FAILURE);
        }

        // Read the OS name from boot sector
        char* os_name = (char*)calloc(OS_NAME_LENGTH_BYTES + 1, sizeof(char)); // Using calloc to avoid VLA
        if (os_name == NULL) {
                perror("Memory allocation failed");
                fclose(file);
                exit(EXIT_FAILURE);
        }
        if (fread(os_name, 1, OS_NAME_LENGTH_BYTES, file) != OS_NAME_LENGTH_BYTES) { // Read one byte at a time into os_name for OS_NAME_LENGTH_BYTES number of times
                perror("Error reading OS name");
                fclose(file);
                exit(EXIT_FAILURE);
        }

        return os_name;

}

// Function to check whether the char array for the label has changed from when it was initialized to all zeros
int is_label_unchanged (char* label) {

	for (int i = 0; i < LABEL_LENGTH_BYTES; i++) {
		if (label[i] != 0) {
			return 0; // False, label is changed
		}
	}
	return 1; // True, label is unchanged

}

// Function to get the label of the provided disk image
char* get_label(FILE* file) {

	// Seek to position of label in boot sector
        if (fseek(file, LABEL_START_BYTE, SEEK_SET) != 0) { // Move file pointer LABEL_START_BYTE number of bytes relative to start of file
                perror("Error seeking to label position of boot sector");
                fclose(file);
                exit(EXIT_FAILURE);
        }

        // Read the label from boot sector
        char* label = (char*)calloc(LABEL_LENGTH_BYTES + 1, sizeof(char)); // Using calloc to avoid VLA
        if (label == NULL) {
                perror("Memory allocation failed");
                fclose(file);
                exit(EXIT_FAILURE);
        }
        if (fread(label, 1, LABEL_LENGTH_BYTES, file) != LABEL_LENGTH_BYTES) { // Read one byte at a time into label for LABEL_LENGTH_BYTES number of times
                perror("Error reading label");
                fclose(file);
                exit(EXIT_FAILURE);
        }

	// If label not found, then check root directory
        if (!is_label_unchanged(label)) {

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

// Function to get the total sector count of the provided disk image
uint16_t get_total_sector_count(FILE* file) {

	// Seek to position of total sector count in boot sector
	if (fseek(file, TOTAL_SECTOR_COUNT_START_BYTE, SEEK_SET) != 0) { // Move file pointer TOTAL_SECTOR_COUNT_START_BYTE number of bytes relative to start of file
		perror("Error seeking to total sector count position of boot sector");
		fclose(file);
		exit(EXIT_FAILURE);
	}

	// Read the total sector count from boot sector
	uint16_t total_sector_count;
	if (fread(&total_sector_count, 1, TOTAL_SECTOR_COUNT_LENGTH_BYTES, file) != TOTAL_SECTOR_COUNT_LENGTH_BYTES) { // Read one byte at a time into total_sector_count for TOTAL_SECTOR_COUNT_LENGTH_BYTES number of times
		perror("Error reading total sector count");
		fclose(file);
		exit(EXIT_FAILURE);
	}

	return total_sector_count;

}

// Function to get the unused sector count of the provided disk image
int get_unused_sector_count(FILE* file) {

	uint16_t total_sector_count = get_total_sector_count(file);
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

// Function to get the number of sectors per FAT in the provided disk image
uint16_t get_sectors_per_fat(FILE* file){

        uint16_t sectors_per_fat = 0;

        // Seek to position of sectors per FAT in boot sector
        if (fseek(file, SECTORS_PER_FAT_START_BYTE, SEEK_SET) != 0) { // Move file pointer SECTORS_PER_FAT_START_BYTE number of bytes relative to start of file
                perror("Error seeking to sectors per FAT position of boot sector");
                fclose(file);
                exit(EXIT_FAILURE);
        }

        // Read the sectors per FAT from boot sector
        if (fread(&sectors_per_fat, 1, SECTORS_PER_FAT_LENGTH_BYTES, file) != SECTORS_PER_FAT_LENGTH_BYTES) { // Read one byte at a time into sectors_per_fat for SECTORS_PER_FAT_LENGTH_BYTES number of times
                perror("Error reading sectors per FAT");
                fclose(file);
                exit(EXIT_FAILURE);
        }

        return sectors_per_fat;

}

// Function to get the number of FAT copies in the provided disk image
uint8_t get_num_fat_copies(FILE* file){

        uint8_t num_fat_copies = 0;

        // Seek to position of number of FAT copies in boot sector
        if (fseek(file, NUM_FAT_COPIES_START_BYTE, SEEK_SET) != 0) { // Move file pointer NUM_FAT_COPIES_START_BYTE number of bytes relative to start of file
                perror("Error seeking to number of FAT copies position of boot sector");
                fclose(file);
                exit(EXIT_FAILURE);
        }

        // Read the number of FAT copies from boot sector
        if (fread(&num_fat_copies, 1, NUM_FAT_COPIES_LENGTH_BYTES, file) != NUM_FAT_COPIES_LENGTH_BYTES) { // Read one byte at a time into num_fat_copies for NUM_FAT_COPIES_LENGTH_BYTES number of times
                perror("Error reading number of FAT copies");
                fclose(file);
                exit(EXIT_FAILURE);
        }

        return num_fat_copies;

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
	char* os_name = get_os_name(file);

	// Get the label
	char* label = get_label(file);

	// Calculate the total size
	float total_size = get_total_sector_count(file) * SECTOR_SIZE_BYTES;

        // Calculate the free size
	float free_size = get_unused_sector_count(file) * SECTOR_SIZE_BYTES;
	
        // Get the number of sectors per FAT
        uint16_t sectors_per_fat = get_sectors_per_fat(file);

	// Get the number of FAT copies
	uint8_t num_fat_copies = get_num_fat_copies(file);

	fprintf(stdout, "OS Name: %s\n", os_name);
	fprintf(stdout, "Label of the disk: %s\n", label);
	fprintf(stdout, "Total size of the disk: %.0f\n", total_size);
	fprintf(stdout, "Free size of the disk: %.0f\n", free_size);
	fprintf(stdout, "Number of sectors per FAT: %" PRIu16 "\n", sectors_per_fat);
	fprintf(stdout, "Number of FAT copies: %" PRIu8 "\n", num_fat_copies);

	free(label);
	free(os_name);
	return 0;

}
