#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

int decompress()
{
    FILE *stdin_handle  = fdopen(0, "rb");
	FILE *stdout_handle = fdopen(1, "w");


	int byte;
	while ((byte = fgetc(stdin_handle)) != EOF) {
		size_t count = byte & 0x7F;

		if (byte & 0x80) {
			byte = fgetc(stdin_handle);
			if (byte == EOF) goto error;

			for (size_t i = 0; i < count; i++) fputc(byte, stdout_handle);
		} else {
			for (size_t i = 0; i < count; i++) {
				byte = fgetc(stdin_handle);
				if (byte == EOF) goto error;

				fputc(byte, stdout_handle);
			}
		}
	}

	fclose(stdin_handle);
	fclose(stdout_handle);
	return 0;

error:

	dprintf(2, "Malformatted honk file!");

	fclose(stdin_handle);
	fclose(stdout_handle);
	return -1;
}

enum compression_state {
	META,
	HOMOGENEOUS,
	HETEROGENEOUS
};

void output_het(char *buffer, FILE *stdout_handle) {
	unsigned char status = strlen(buffer);
	if (status == 0x00) return;

	fwrite(&status, 1, 1, stdout_handle);
	fwrite(buffer, strlen(buffer), 1, stdout_handle);
}

void output_hom(char c, int count, FILE *stdout_handle) {
	unsigned char status = 0x80 | count;

	fwrite(&status, 1, 1, stdout_handle);
	fputc(c, stdout_handle);
}

int compress()
{
    FILE *stdin_handle  = fdopen(0, "rb");
	FILE *stdout_handle = fdopen(1, "wb");

	enum compression_state current_state = META;

	char buffer[128];
	memset(buffer, 0, 128);

	size_t count = 0;

	char x = fgetc(stdin_handle);
	char x_1 = fgetc(stdin_handle);

	while (x != EOF) {
		switch (current_state) {
		case META:
			count = 0;
			memset(buffer, 0, 128);

			if (x == x_1) {
				count++;
				current_state = HOMOGENEOUS;
			} else {
				buffer[count++] = x;
				current_state = HETEROGENEOUS;
			}
			break;
		case HOMOGENEOUS:
			count++;
			if (x == x_1) {
				if (count >= 127) {
					output_hom(x, count, stdout_handle);
					current_state = META;
				}
			} else {
				output_hom(x, count, stdout_handle);
				current_state = HETEROGENEOUS;
				count = 0;
				memset(buffer, 0, 128);
			}
			break;
		case HETEROGENEOUS:
			if (x != x_1) {
				buffer[count++] = x;

				if (count >= 127 || x_1 == EOF) {
					output_het(buffer, stdout_handle);
					current_state = META;
				}
			} else {
				output_het(buffer, stdout_handle);
				memset(buffer, 0, 128);
				current_state = HOMOGENEOUS;
				count = 1;
			}
			break;
		}

		x = x_1;
		x_1 = fgetc(stdin_handle);
	}

    fclose(stdin_handle);
	fclose(stdout_handle);
	return 0;
}

int main(int argc, char **argv)
{
    int opt;
    while ((opt = getopt(argc, argv, "d")) != -1) {
        switch (opt) {
            case 'd':
                return decompress();
        }
    }

	return compress();
}
