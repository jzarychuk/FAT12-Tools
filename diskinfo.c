#include <stdio.h>
#include <stdlib.h>

#define OS_NAME_START_BYTE 3
#define OS_NAME_LENGTH_BYTES 8

int main (int argc, char* argv[]) {

	if (argc < 2) {
		fprintf(stderr, "Usage: %s <disk.IMA>\n", argv[0]);
		exit(2);
	}

	// Open the provided file
	FILE* file = fopen(argv[1], "r");
	if (file == NULL) {
		perror("Error opening file");
		exit(EXIT_FAILURE);
	}

	// Seek to the position of OS name in the boot sector
	if (fseek(file, OS_NAME_START_BYTE, SEEK_SET) != 0) { // Move file pointer OS_NAME_START number of bytes relative to start of file
		perror("Error seeking to OS name position of boot sector");
		fclose(file);
		exit(EXIT_FAILURE);
	}

	// Read the OS name
	char os_name[OS_NAME_LENGTH_BYTES + 1] = {0};
	if (fread(os_name, 1, OS_NAME_LENGTH_BYTES, file) != OS_NAME_LENGTH_BYTES) { // Read one byte at a time into os_name for OS_NAME_LENGTH times
		perror("Error reading OS name");
		fclose(file);
		exit(EXIT_FAILURE);
	}

	fprintf(stdout, "OS Name: %s\n", os_name);

	return 0;
}
