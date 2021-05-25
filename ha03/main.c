#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <time.h>

#include <math.h>

#include <error.h>
#include <errno.h>

#define BUFFER_SIZE 1024
#define LOADING_BAR_LEN 100
#define RELOAD_BAR 1500

// create a temporary file in the same dict with random name
// write the whole stdin into that file
inline static int save_to_temp(char *path_name)
{
	FILE *stdin_handle  = fdopen(0, "rb");

	// create file with random name
	memset(path_name, 0, 100);
	srandom(time(0));
	sprintf(path_name, "%ld", random());

	FILE *tmp = fopen(path_name, "wb"); //temp file

	if (tmp == NULL) goto error;

	dprintf(2, "reading...\n"); //printig reading message

	// copy stdin into that file
	char buffer[BUFFER_SIZE];
	size_t read_len;
	do {
		memset(buffer, 0, BUFFER_SIZE);
		read_len  = fread(buffer, 1, BUFFER_SIZE, stdin_handle);
		fwrite(buffer, 1, read_len, tmp);
		fflush(tmp);
	} while (read_len > 0);

	if (ferror(stdin_handle) || ferror(tmp)) goto error;

	// close the streams
	fclose(stdin_handle);
	fclose(tmp);

	return 1;
error:
	// error output
	dprintf(2, "Error %s\n", strerror(errno));
	sprintf(path_name, "%s", "\0");

	fclose(tmp);
	fclose(stdin_handle);
	return 0;
}


size_t counter = 0; // needed for slowing down the progress bar

// prints a progress bar after this function has been called RELOAD_BAR times
inline static void print_progress(FILE *file, size_t file_len)
{
	counter++;
	if (counter % RELOAD_BAR == 0) {
		// get current position in file
		size_t current_pos = ftell(file);

		// calculate progress
		double perc = (double)current_pos / file_len;
		size_t arrow_count = (size_t)(LOADING_BAR_LEN * perc);

		// output loading bar
		dprintf(2, "Progress: %.2f%% ", perc * 100.0);
		dprintf(2, "[");
		for (int i = 0; i < arrow_count; i++) dprintf(2, ">");
		for (int i = 0; i < LOADING_BAR_LEN - arrow_count; i++) dprintf(2, " ");
		dprintf(2, "]");
		dprintf(2, "\r");
	}
}

// delete the tempfile
static inline void free_temp_file(char *path_name) {
	remove(path_name);
}

// read out the file size of the gíven file pointer
inline static size_t get_file_size(FILE *file)
{
	fseek(file, 0L, SEEK_END);
	size_t sz = ftell(file);
	fseek(file, 0L, SEEK_SET);

	return sz;
}

int decompress()
{
	// save stdin into tempfile
	char file_name[100];
	int result = save_to_temp(file_name);

	// open input/output streams
	FILE *temp = fopen(file_name, "rb");
	// if tempfile couldn't be created fall back to reading from stdin directly
    FILE *input_handle  = (result != 0 && temp != NULL) ? temp : fdopen(0, "rb");
	FILE *stdout_handle = fdopen(1, "w");

	dprintf(2, "decompressing...\n");

	// if tempfile is present => read in its contents
	size_t file_len;
	if(result) file_len = get_file_size(input_handle);

	int byte;

	// iterate over all bytes
	while ((byte = fgetc(input_handle)) != EOF) {
		// get count information of byte (byte without msb)
		size_t count = byte & 0x7F;

		// msb == 1 => get next byte and put next byte count times into output
		if (byte & 0x80) {
			byte = fgetc(input_handle);
			if (byte == EOF) goto error;

			for (size_t i = 0; i < count; i++) fputc(byte, stdout_handle);
		// msb == 0 => copy the next count bytes directly into the output
		} else {
			for (size_t i = 0; i < count; i++) {
				byte = fgetc(input_handle);
				if (byte == EOF) goto error;

				fputc(byte, stdout_handle);
			}
		}

		// progress bar
		if (temp != NULL) print_progress(input_handle, file_len);
	}

	// close the streams
	fclose(input_handle);
	fclose(stdout_handle);
	if (result != 0) free_temp_file(file_name);
	return 0;

error:

	// error handling
	dprintf(2, "Malformatted honk file!");

	fclose(input_handle);
	fclose(stdout_handle);
	if (result != 0) free_temp_file(file_name);

	return -1;
}

// states for state machine of compression algorithm
enum compression_state {
	UNDECIDED,
	HOMOGENEOUS,
	HETEROGENEOUS
};

// copy the character directly into the output stream
static inline void put_byte(char c, FILE *stdout_handle)
{
	fwrite(&c, 1, 1, stdout_handle);
}

// copy the whole buffer into the output stream together with lenght info
void output_het(char *buffer, size_t count, FILE *stdout_handle)
{
	unsigned char status = count;
	if (status == 0x00) return;

	put_byte(status, stdout_handle);
	fwrite(buffer, 1, count, stdout_handle);
}

// copy the character and repetition information into the output
void output_hom(char c, int count, FILE *stdout_handle)
{
	unsigned char status = 0x80 | count;

	put_byte(status, stdout_handle);
	put_byte(c, stdout_handle);
}

int compress()
{
	// save stdin into tempfile
	char file_name[100];
    int result = save_to_temp(file_name);

	// open input/output streams
	FILE *temp = fopen(file_name, "rb");
	// if tempfile couldn't be created fall back to reading from stdin directly
    FILE *input_handle  = (temp) ? temp : fdopen(0, "rb");
	FILE *stdout_handle = fdopen(1, "wb");

	dprintf(2, "compressing...\n");

	// if tempfile is present => read in its contents
	size_t file_len;
	if(result) file_len = get_file_size(input_handle);

	enum compression_state current_state = UNDECIDED;

	char buffer[128];
	memset(buffer, 0, 128);

	// needed for don't overflowing the buffer
	// maximal encoding length == 127 byte
	size_t count = 0;

	// current and next byte
	uint32_t x = fgetc(input_handle);
	uint32_t x_1 = fgetc(input_handle);

	// iterate over all bytes from the input
	while (x != EOF) {
		switch (current_state) {

		// if current state is not known => check if current and next byte are equal
		// if they are => set state to homogenous and set vars accordingly
		// if they aren't => set state to heterogenous
		case UNDECIDED:
			count = 0;
			memset(buffer, 0, 128);

			if (x == x_1) {
				count++;
				current_state = HOMOGENEOUS;
			} else {
				buffer[count++] = x & 0xFF;
				current_state = HETEROGENEOUS;
			}
			break;
		case HOMOGENEOUS:
			count++;
			// if next 2 bytes are not equal or buffer full or end of file
			// write to output and set undecided state
			// b/c next 2 bytes could be equal again

			if (x != x_1 || count >= 127 || x_1 == EOF) {
				output_hom(x, count, stdout_handle);
				current_state = UNDECIDED;
			}
			break;
		case HETEROGENEOUS:
			// if next to bytes are not equal
			if (x != x_1) {
				// save current byte into buffer
				buffer[count++] = x & 0xFF;

				// if buffer full or end of file => write output
				// set undecided state
				if (count >= 127 || x_1 == EOF) {
					output_het(buffer, count, stdout_handle);
					current_state = UNDECIDED;
				}
			// if they are equal => write output and set homogenous state
			} else {
				output_het(buffer, count, stdout_handle);
				current_state = HOMOGENEOUS;
				count = 1;
			}
			break;
		}

		// advance loop
		x = x_1;
		x_1 = fgetc(input_handle);

		// print progress
		if (temp != NULL) print_progress(input_handle, file_len);
	}

	// close streams
    fclose(input_handle);
	fclose(stdout_handle);

	// delete tempfile if present
	if (result != 0) free_temp_file(file_name);

	return 0;
}

int main(int argc, char **argv)
{

	// if programs gets called with -d switch => decompress
	// else compress data from stdin
    int opt;
    while ((opt = getopt(argc, argv, "d")) != -1) {
        switch (opt) {
            case 'd':
                return decompress();
        }
    }

	return compress();
}
