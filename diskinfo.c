#include "fat12_utils.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

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
                        entry = find_entry(file, 0x08, root_dir_start_byte + sector_offset);
                        if (entry != NULL) {
                                strncpy(label, entry, 11); // Copy the first 11 characters of the entry into the label
                                free(entry);
                                break;
                        }
                }

        }		

	return label;

}

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

	fprintf(stdout, "OS Name: %s\n", os_name);
	fprintf(stdout, "Label of the disk: %s\n", label);

	free(label);
	free(os_name);
	return 0;

}
