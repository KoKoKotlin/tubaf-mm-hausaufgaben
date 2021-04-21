#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>

#include "lib/bitmap.h"

#define MIN(x, y) ((x > y) ? y : x)

// alpha blending of two RGB bitmaps
void manipulate(bitmap_pixel_rgb_t *pixels1, bitmap_pixel_rgb_t *pixels2,
                uint32_t width1, uint32_t height1,
                uint32_t width2, uint32_t height2,
                double alpha)
{
    // calculation borders for blending (diffrent size of bitmaps)
    uint32_t min_width = MIN(width1, width2);
    uint32_t min_height = MIN(height1, height2);

    // blending pixels together
    for (uint32_t y = 0; y < min_height; y++) {
        for (uint32_t x = 0; x < min_width; x++) {
            uint32_t index1 = y * width1 + x;
            uint32_t index2 = y * width2 + x;

            bitmap_pixel_rgb_t *pixel1 = &pixels1[index1];
            bitmap_pixel_rgb_t *pixel2 = &pixels2[index2];
            
            pixel1->r = (pixel1->r * alpha + (1 - alpha) * pixel2->r);
            pixel1->g = (pixel1->g * alpha + (1 - alpha) * pixel2->g);
            pixel1->b = (pixel1->b * alpha + (1 - alpha) * pixel2->b);
        }
    }
}

// reading two bitmaps, calling alpha blending and writing back pixles
bitmap_error_t alpha_blend(char *file_path1, char *file_path2, char *output_file_path, double alpha_blend)
{
	// read the bitmap pixels
	bitmap_error_t error1, error2;
	uint32_t width1, width2, height1, height2;
	bitmap_pixel_rgb_t *pixels1;
    bitmap_pixel_rgb_t *pixels2;

    // error for bitmap #1
	error1 = bitmapReadPixels(
		file_path1,
		(bitmap_pixel_t**)&pixels1,
		&width1,
		&height1,
		BITMAP_COLOR_SPACE_RGB
	);

    // error for bitmap #2
    error2 = bitmapReadPixels(
		file_path2,
		(bitmap_pixel_t**)&pixels2,
		&width2,
		&height2,
		BITMAP_COLOR_SPACE_RGB
	);

    // handling bitmap reading errors
    if (error1 != BITMAP_ERROR_SUCCESS || error2 != BITMAP_ERROR_SUCCESS) goto free_res;

    // calling alpha blendig
    manipulate(pixels1, pixels2, width1, height1, width2, height2, alpha_blend);
	
	// write the pixels back
	bitmap_parameters_t params =
	{
		.bottomUp = BITMAP_BOOL_TRUE,
		.widthPx = width1,
		.heightPx = height1,
		.colorDepth = BITMAP_COLOR_DEPTH_24,
		.compression = BITMAP_COMPRESSION_NONE,
		.dibHeaderFormat = BITMAP_DIB_HEADER_INFO,
		.colorSpace = BITMAP_COLOR_SPACE_RGB
	};

    // error handling for writing pixels
	error1 = bitmapWritePixels(
		output_file_path,
		BITMAP_BOOL_TRUE,
		&params,
		(bitmap_pixel_t*)pixels1
	);

free_res:
	// free the memory that has been allocated by the bitmap library
	free(pixels1);
    free(pixels2);
    return (error1 != 0) ? error1 : error2;;
}

int main(int argc, char** argv)
{   
    // getting arguments from terminal input
    int opt;
    char *alpha_blending_str = "0.5";
    char *new_file_path = "out.bmp";

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

    double alpha = atof(alpha_blending_str);

    // error handling for alpha input
    if (alpha < 0.0 || alpha > 1.0) {
        fprintf(stderr, "The alpha was not in the valid range from 0.0 to 1.0! Exiting...");
        return 1;
    }

    // error handling for bitmap input
    bitmap_error_t error;
    char *bmp1 = NULL;
    char *bmp2 = NULL;

    // read in 2 files from the command line arguments
    for (uint32_t index = optind; index < argc; index++) {
        if(bmp1 == NULL) bmp1 = argv[index];
        else             bmp2 = argv[index];
    }

    // error detection if 2 files were given
    if (bmp1 == NULL || bmp2 == NULL) {
        fprintf(stderr, "The program needs 2 file paths as input!");
        return 1;
    }

    error = alpha_blend(bmp1, bmp2, new_file_path, alpha);

    // error handling for alpha blending
    switch (error) {
        case BITMAP_ERROR_INVALID_PATH:
            fprintf(stderr, "At least one file path is invalid!\n");
            break;
        case BITMAP_ERROR_INVALID_FILE_FORMAT:
            fprintf(stderr, "At least one file is not in the right file format!\n");
            break;
        case BITMAP_ERROR_IO:
            fprintf(stderr, "Error while reading at least one file.\n");
            break;
        case BITMAP_ERROR_MEMORY:
            fprintf(stderr, "Error while writing file.\n");
            break;
        case BITMAP_ERROR_FILE_EXISTS:
            fprintf(stderr, "Error: File does already exist.\n");
            break;
    }
    
	return 0;
}
