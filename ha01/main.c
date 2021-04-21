#include <assert.h>
#include <limits.h>
#include <stdint.h>
#include <stdlib.h>

#include "lib/bitmap.h"

void manipulate(bitmap_pixel_hsv_t* pixels, uint32_t width, uint32_t height)
{   
    int offset = 50;

    for (uint32_t i = 0; i < width * height; i++) {
        bitmap_pixel_hsv_t* current_pixel = &pixels[i];
    
        int pixel_value_v = (int)(current_pixel->v);
        pixel_value_v += offset;

        if (pixel_value_v > 255)     pixel_value_v = 255;
        else if (pixel_value_v < 0)  pixel_value_v = 0;
        
        current_pixel->v = (bitmap_component_t)pixel_value_v;
    }
}

int main(void)
{
	// Read the bitmap pixels.
	bitmap_error_t error;
	uint32_t width, height;
	bitmap_pixel_hsv_t* pixels;

	error = bitmapReadPixels(
		"sails.bmp",
		(bitmap_pixel_t**)&pixels,
		&width,
		&height,
		BITMAP_COLOR_SPACE_HSV
	);

	assert(error == BITMAP_ERROR_SUCCESS);

	// Manipulate the pixels.
	manipulate(pixels, width, height);

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

	error = bitmapWritePixels(
		"sails_mod.bmp",
		BITMAP_BOOL_TRUE,
		&params,
		(bitmap_pixel_t*)pixels
	);

	assert(error == BITMAP_ERROR_SUCCESS);

	// Free the memory that has been allocated by the bitmap library.
	free(pixels);

	return 0;
}
