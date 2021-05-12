#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

int decompress()
{
    FILE *stdin_handle  = fdopen(0, "r");
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

int compress()
{
    FILE *stdin_handle  = fdopen(0, "r");
	FILE *stdout_handle = fdopen(1, "w");

    //TODO

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
            default:
                return compress();
        }
    }
}
