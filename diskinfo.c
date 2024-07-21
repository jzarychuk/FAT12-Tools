#include <stdio.h>
#include <stdlib.h>

const int OS_NAME_START_BYTE = 3;
const int OS_NAME_LENGTH_BYTES = 8;

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

	fprintf(stdout, "OS Name: %s\n", os_name);

	return 0;
}
