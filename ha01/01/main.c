#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "lib/bitmap.h"

#define MAX(x, y) ((x < y) ? y : x) // getting the maximum of two values
#define MIN(x, y) ((x > y) ? y : x) // getting the minimum of two values

// manipulating the brightnes of a bitmap (HSV)
void manipulate(bitmap_pixel_hsv_t *pixels, uint32_t width, uint32_t height, int offset)
{
    for (uint32_t i = 0; i < width * height; i++) {
        bitmap_pixel_hsv_t *current_pixel = &pixels[i];

        int pixel_value_v = (int)(current_pixel->v);
        pixel_value_v += offset;

        pixel_value_v = MIN(255, MAX(0, pixel_value_v));

        current_pixel->v = (bitmap_component_t)pixel_value_v;
    }
}

// creating new file path using strncat() (file_path + darker/brigther + offset)
void create_new_filename(char *old_file_path, int offset, char *modified_file_path)
{
    strncpy(modified_file_path, old_file_path, strlen(old_file_path) - 4);
    strncat(modified_file_path, "_", 255);

    if (offset >= 0) strncat(modified_file_path, "brighter", 255);
    else             strncat(modified_file_path, "darker", 255);
    strncat(modified_file_path, "_", 255);

    char offset_buffer[4];
    sprintf(offset_buffer, "%d", abs(offset));
    strncat(modified_file_path, offset_buffer, 255);

    strncat(modified_file_path, ".bmp", 255);
}

// reading, calling manipulate function and writing pixels back
bitmap_error_t brighten_image(char *file_path, int offset)
{
    // read the bitmap pixels
    bitmap_error_t error;
    uint32_t width, height;
    bitmap_pixel_hsv_t *pixels;

    error = bitmapReadPixels(
        file_path,
        (bitmap_pixel_t**)&pixels,
        &width,
        &height,
        BITMAP_COLOR_SPACE_HSV
    );

    // if the bitmap read returns an error, skip the manipulation of the image
    if (error != BITMAP_ERROR_SUCCESS) goto free_res;

    // manipulate the pixels
    manipulate(pixels, width, height, offset);

    // get the new filename
    char modified_file_path[256];
    create_new_filename(file_path, offset, modified_file_path);

    // parameters of the written image
    bitmap_parameters_t params = {
        .bottomUp = BITMAP_BOOL_TRUE,
        .widthPx = width,
        .heightPx = height,
        .colorDepth = BITMAP_COLOR_DEPTH_24,
        .compression = BITMAP_COMPRESSION_NONE,
        .dibHeaderFormat = BITMAP_DIB_HEADER_INFO,
        .colorSpace = BITMAP_COLOR_SPACE_HSV
    };

    // write the pixels back
    error = bitmapWritePixels(
        modified_file_path,
        BITMAP_BOOL_TRUE,
        &params,
        (bitmap_pixel_t*)pixels
    );

free_res:
    // free the memory that has been allocated by the bitmap library
    free(pixels);
    return error;
}

void print_help()
{
    printf("Usage: ./brightness_changer.out fileName1 [fileName2 ... fileNameN] -b brightness_offset\n"
           "The specified files should be bitmap files and the brightness_offset must be specified and should be in the range from -100 to 100 and not be equal to 0!\n"
           "The output of the program are bitmap files with the original name and additional information that shows how they were altered.\n");
}

int main(int argc, char **argv)
{
    // getting the arguments from terminal input
    int opt;
    char *offset_str;
    char *load_file_path;

    while ((opt = getopt(argc, argv, "b:")) != -1) {
        switch (opt) {
            case 'b':
                offset_str = optarg;
                break;
            // triggered on unrecognized option
            case '?':
                fprintf(stderr, "Unrecognized option -%c!\n", optopt);
                print_help();
                return -1;
        }
    }

    int offset = atoi(offset_str);

    // error handling for offset error
    if (offset == 0) {
        fprintf(stderr, "The offset was 0 or invalid! Exiting...\n\n");
        print_help();
        return 1;
    } else if (offset < -100 || offset > 100) {
        fprintf(stderr, "The offset was not in the valid range from -100 to 100! Exiting...\n");
        return 1;
    }

    // error handling for bitmap errors
    bitmap_error_t error;
    for (uint32_t index = optind; index < argc; index++) {
        load_file_path = argv[index];

        error = brighten_image(load_file_path, offset);

        switch (error) {
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
