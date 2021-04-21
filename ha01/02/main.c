#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "lib/bitmap.h"

#define MAX(x, y) ((x < y) ? y : x)
#define MIN(x, y) ((x > y) ? y : x)

void manipulate(bitmap_pixel_hsv_t* pixels, uint32_t width, uint32_t height, int offset)
{
    for (uint32_t i = 0; i < width * height; i++) {
        bitmap_pixel_hsv_t* current_pixel = &pixels[i];
    
        int pixel_value_v = (int)(current_pixel->v);
        pixel_value_v += offset;

        pixel_value_v = MIN(255, MAX(0, pixel_value_v));

        current_pixel->v = (bitmap_component_t)pixel_value_v;
    }
}

bitmap_error_t brighten_image(char* file_path, int offset) {
                    
	// Read the bitmap pixels.
	bitmap_error_t error;
	uint32_t width, height;
	bitmap_pixel_hsv_t* pixels;

	error = bitmapReadPixels(
		file_path,
		(bitmap_pixel_t**)&pixels,
		&width,
		&height,
		BITMAP_COLOR_SPACE_HSV
	);

    if(error != BITMAP_ERROR_SUCCESS) {
	    free(pixels);
        return error;
    }

	// Manipulate the pixels.
	manipulate(pixels, width, height, offset);

	// Write the pixels back.
	bitmap_parameters_t params =
	{
		.bottomUp = BITMAP_BOOL_TRUE,
		.widthPx = width,
		.heightPx = height,
		.colorDepth = BITMAP_COLOR_DEPTH_24,
		.compression = BITMAP_COMPRESSION_NONE,
		.dibHeaderFormat = BITMAP_DIB_HEADER_INFO,
		.colorSpace = BITMAP_COLOR_SPACE_HSV
	};

    // file_path + darker/brigther + offset
    char modified_file_path[256];
    strncpy(modified_file_path, file_path, strlen(file_path) - 4);
    strncat(modified_file_path, "_", 255);
    
    if(offset >= 0) strncat(modified_file_path, "brighter", 255);
    else            strncat(modified_file_path, "darker", 255);
    strncat(modified_file_path, "_", 255);

    char offset_buffer[4];
    sprintf(offset_buffer, "%d", abs(offset));
    strncat(modified_file_path, offset_buffer, 255);

    strncat(modified_file_path, ".bmp", 255);

	error = bitmapWritePixels(
		modified_file_path,
		BITMAP_BOOL_TRUE,
		&params,
		(bitmap_pixel_t*)pixels
	);

	// Free the memory that has been allocated by the bitmap library.
	free(pixels);
    return error;
}

int main(int argc, char** argv)
{
    int opt;
    char* alpha_blending_str;
    char* new_file_path = "out.bmp";

    while ((opt = getopt(argc, argv, "a:o:")) != -1) {
        switch (opt) {
            case 'a':
                alpha_blending_str = optarg; 
                break;
            case 'o':
                new_file_path = optarg;
                break;
        }
    }
    
    int offset = atoi(offset_str);

    if (offset == 0) 
    {
        fprintf(stderr, "The offset was 0 or invalid! Exiting...");
        return 1;
    } else if (offset < -100 || offset > 100) 
    {
        fprintf(stderr, "The offset was not in the valid range from -100 to 100! Exiting...");
        return 1;
    }

    bitmap_error_t error;
    for (uint32_t index = optind; index < argc; index++) {
        load_file_path = argv[index];
        
        error = brighten_image(load_file_path, offset);

        switch(error) {
            case BITMAP_ERROR_INVALID_PATH:
                fprintf(stderr, "The file path (%s) does not exist.\n", load_file_path);
                break;
            case BITMAP_ERROR_INVALID_FILE_FORMAT:
                fprintf(stderr, "The file (%s) is not in the right format.\n", load_file_path);
                break;
            case BITMAP_ERROR_IO:
                fprintf(stderr, "Error while reading file (%s).\n", load_file_path);
                break;
            case BITMAP_ERROR_MEMORY:
                fprintf(stderr, "Error while writing file.\n");
                break;
            case BITMAP_ERROR_FILE_EXISTS:
                fprintf(stderr, "Error: File does already exist.\n");
                break;
        }
    }

	return 0;
}
