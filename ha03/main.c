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

//Tempfile
inline static int save_to_temp(char *path_name) 
{
	FILE *stdin_handle  = fdopen(0, "rb");
	
	memset(path_name, 0, 100);
	srandom(time(0));
	sprintf(path_name, "%ld", random());
	
	FILE *tmp = fopen(path_name, "wb"); //temp file
	
	if (tmp == NULL) goto error;

	char buffer[BUFFER_SIZE];

	dprintf(2, "reading...\n"); //printig reading message

	size_t read_len;
	do {
		memset(buffer, 0, BUFFER_SIZE);
		read_len  = fread(buffer, 1, BUFFER_SIZE, stdin_handle);
		fwrite(buffer, 1, read_len, tmp);
		fflush(tmp);
	} while (read_len > 0);
	
	if (ferror(stdin_handle) || ferror(tmp)) goto error;

	fclose(stdin_handle);
	fclose(tmp);
	
	return 1;
error:
	// error output to do
	dprintf(2, "Error %s\n", strerror(errno));
	sprintf(path_name, "%s", "\0");

	fclose(tmp);
	fclose(stdin_handle);
	return 0;
}

size_t counter = 0;
inline static void print_progress(FILE *file, size_t file_len)
{		
	counter++;
	if (counter % RELOAD_BAR == 0) {
		size_t current_pos = ftell(file);
		double perc = (double)current_pos / file_len;

		size_t arrow_count = (size_t)(LOADING_BAR_LEN * perc);
		
		dprintf(2, "Progress: %.2f%% ", perc * 100.0);
		dprintf(2, "[");
		for (int i = 0; i < arrow_count; i++) dprintf(2, ">");
		for (int i = 0; i < LOADING_BAR_LEN - arrow_count; i++) dprintf(2, " ");
		dprintf(2, "]");
		dprintf(2, "\r");
	}
}

static inline void free_temp_file(char *path_name) {
	remove(path_name);
}

inline static size_t get_file_size(FILE *file)
{
	fseek(file, 0L, SEEK_END);
	size_t sz = ftell(file);
	fseek(file, 0L, SEEK_SET);

	return sz;
} 

int decompress()
{	
	//reading byte per byte
	char file_name[100];
	int result = save_to_temp(file_name);

	FILE *temp = fopen(file_name, "rb");
    FILE *input_handle  = (result != 0 && temp != NULL) ? temp : fdopen(0, "rb");
	FILE *stdout_handle = fdopen(1, "w");

	//printig terminal output
	dprintf(2, "decompressing...\n");
	
	//getting file length for progress bar
	size_t file_len;
	if(result == 0) file_len = get_file_size(input_handle);

	int byte;

	while ((byte = fgetc(input_handle)) != EOF) {
		size_t count = byte & 0x7F;

		if (byte & 0x80) { //homogeneous 
			byte = fgetc(input_handle);
			if (byte == EOF) goto error;

			for (size_t i = 0; i < count; i++) fputc(byte, stdout_handle);
		} else { //heterogeneous
			for (size_t i = 0; i < count; i++) {
				byte = fgetc(input_handle);
				if (byte == EOF) goto error;

				fputc(byte, stdout_handle);
			}
		}

		if (temp != NULL) print_progress(input_handle, file_len);
	}

	fclose(input_handle);
	fclose(stdout_handle);
	if (result != 0) free_temp_file(file_name);
	return 0;

error:

	dprintf(2, "Malformatted honk file!");

	fclose(input_handle);
	fclose(stdout_handle);
	if (result != 0) free_temp_file(file_name);
	
	return -1;
}

enum compression_state {
	UNDECIDED,
	HOMOGENEOUS,
	HETEROGENEOUS
};

static inline void put_byte(char c, FILE *stdout_handle)
{
	fwrite(&c, 1, 1, stdout_handle);
}

void output_het(char *buffer, size_t count, FILE *stdout_handle)
{
	unsigned char status = count;
	if (status == 0x00) return;

	put_byte(status, stdout_handle);
	fwrite(buffer, 1, count, stdout_handle);
}

void output_hom(char c, int count, FILE *stdout_handle)
{
	unsigned char status = 0x80 | count;

	put_byte(status, stdout_handle);
	put_byte(c, stdout_handle);
}

int compress()
{	
	
	char file_name[100];
    int result = save_to_temp(file_name);

	FILE *temp = fopen(file_name, "rb");
    FILE *input_handle  = (temp) ? temp : fdopen(0, "rb");
	FILE *stdout_handle = fdopen(1, "wb");

	//printing terminal output
	dprintf(2, "compressing...\n");

	size_t file_len;
	if(result != 0) file_len = get_file_size(input_handle);

	enum compression_state current_state = UNDECIDED;

	char buffer[128];
	memset(buffer, 0, 128);

	size_t count = 0;

	uint32_t x = fgetc(input_handle);
	uint32_t x_1 = fgetc(input_handle);

	while (x != EOF) {
		switch (current_state) {
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
			if (x != x_1 || count >= 127 || x_1 == EOF) {
				output_hom(x, count, stdout_handle);
				current_state = UNDECIDED;
			}
			break;
		case HETEROGENEOUS:
			if (x != x_1) {
				buffer[count++] = x & 0xFF;

				if (count >= 127 || x_1 == EOF) {
					output_het(buffer, count, stdout_handle);
					current_state = UNDECIDED;
				}
			} else {
				output_het(buffer, count, stdout_handle);
				current_state = HOMOGENEOUS;
				count = 1;
			}
			break;
		}

		x = x_1;
		x_1 = fgetc(input_handle);
		if (temp != NULL) print_progress(input_handle, file_len);
	}

    fclose(input_handle);
	fclose(stdout_handle);
	if (result != 0) free_temp_file(file_name);
	
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
