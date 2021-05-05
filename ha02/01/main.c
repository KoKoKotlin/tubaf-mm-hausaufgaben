#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <pmmintrin.h>

#include "lib/bitmap.h"

// manipulating the brightnes of a bitmap (HSV)
void manipulate(bitmap_pixel_hsv_t *pixels, uint32_t width, uint32_t height, float brighten_rate)
{
    float *brightness_buffer = NULL;
    
    uint32_t pixel_count = width * height;
    uint32_t buffer_size = (pixel_count) % 4 == 0 ? pixel_count : pixel_count + (4 - pixel_count % 4);

    posix_memalign((void**)&brightness_buffer, 16, sizeof(float) * buffer_size);
    
    for(size_t i = 0; i < buffer_size; i++)
        if(i < pixel_count) brightness_buffer[i] = (float)pixels[i].v;
        else                brightness_buffer[i] = 0.0f;
    
    
    float *temp_buff = NULL;
    posix_memalign((void**)&temp_buff, 16, sizeof(float) * 4);    
    for(size_t i = 0; i < 4; i++) temp_buff[i] = 255.0f;
    __m128 v_max = _mm_load_ps(temp_buff);
    free(temp_buff);

    posix_memalign((void**)&temp_buff, 16, sizeof(float) * 4);    
    for(size_t i = 0; i < 4; i++) temp_buff[i] = brighten_rate;
    __m128 brightness_rate_sse = _mm_load_ps(temp_buff);
    free(temp_buff);

    // new_v_c = v_c + delta * brighten_rate;
    for (size_t i = 0; i < buffer_size / 4; i++) {
        __m128 current = _mm_load_ps(brightness_buffer + i * 4);
        
        __m128 delta;
        if (brighten_rate < 0) delta = current;
        else                   delta = _mm_sub_ps(v_max, current);
        
        __m128 weigthed_delta = _mm_mul_ps(delta, brightness_rate_sse);
        current = _mm_add_ps(weigthed_delta, current);

        _mm_store_ps(brightness_buffer + 4 * i, current);
    }
    
    for(size_t i = 0; i < buffer_size; i++)
        pixels[i].v = (bitmap_component_t)brightness_buffer[i];
        
    free(brightness_buffer);
}

// creating new file path using strncat() (file_path + darker/brigther + offset)
void create_new_filename(char *old_file_path, float brighten_rate, char *modified_file_path)
{
    strncpy(modified_file_path, old_file_path, strlen(old_file_path) - 4);
    strncat(modified_file_path, "_", 255);

    if (brighten_rate >= 0) strncat(modified_file_path, "brighter", 255);
    else                    strncat(modified_file_path, "darker", 255);
    strncat(modified_file_path, "_", 255);

    char offset_buffer[6];
    memset(offset_buffer, 0, 6);
    sprintf(offset_buffer, "%.2f", fabs(brighten_rate));
    strncat(modified_file_path, offset_buffer, 255);

    strncat(modified_file_path, ".bmp", 255);
}
 
// reading, calling manipulate function and writing pixels back
bitmap_error_t brighten_image(char *file_path, float brighten_rate)
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
    // the buffer is freed in the lib code
    if (error != BITMAP_ERROR_SUCCESS) return error;

    // manipulate the pixels
    manipulate(pixels, width, height, brighten_rate);

    // get the new filename
    char modified_file_path[256];
    create_new_filename(file_path, brighten_rate, modified_file_path);

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
    char *brighten_string;
    char *load_file_path;

    while ((opt = getopt(argc, argv, "b:")) != -1) {
        switch (opt) {
            case 'b':
                brighten_string = optarg;
                break;
            // triggered on unrecognized option
            case '?':
                fprintf(stderr, "Unrecognized option -%c!\n", optopt);
                print_help();
                return -1;
        }
    }

    float brighten_rate = (float)atof(brighten_string);

    // error handling for offset error
    if (brighten_rate < -1.0f || brighten_rate > 1.0f) {
        fprintf(stderr, "The offset was not in the valid range from -1 to 1! Exiting...\n");
        return 1;
    }

    // error handling for bitmap errors
    bitmap_error_t error;
    for (uint32_t index = optind; index < argc; index++) {
        load_file_path = argv[index];

        error = brighten_image(load_file_path, brighten_rate);

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
