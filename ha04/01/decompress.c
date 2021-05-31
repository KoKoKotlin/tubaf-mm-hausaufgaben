#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "dctquant.h"
#include "bitmap.h"

// Perform zig-zag decoding.
static void un_zig_zag(const int8_t* input_block, int8_t* output_block)
{
	for (size_t i = 0; i < 64; i++)
		output_block[i] = input_block[zig_zag_index_matrix[i]];
}


// Multiply with the components of the quantization matrix.
static void dequantize(const int8_t* input_block, const uint32_t* quant_matrix, float* output_block)
{
	for (size_t i = 0; i < 64; i++)
		output_block[i] = ((float)input_block[i] * (float)quant_matrix[i]);
}

static float idct_sum(const float *input_block, uint8_t m, uint8_t n, float cosine_values[8][8])
{
	float sum = 0;

	for (uint8_t q = 0; q < 8; q++)
		for (uint8_t p = 0; p < 8; p++)
			sum += alpha(p) * alpha(q) * input_block[(8 * q) + p] * cosine_values[m][p] * cosine_values[n][q];

	return sum;
}

// Perform the IDCT on the given block of input data.
static void perform_inverse_dct(const float* input_block, float* output_block, float cosine_values[8][8])
{
	for (uint8_t n = 0; n < 8; n++)
		for (uint8_t m = 0; m < 8; m++)
			output_block[8 * n + m] = idct_sum(input_block, m, n, cosine_values);

}

// Copy the block at (`index_x`, `index_y`) from `block` into the given pixel buffer.
// The dimensions of the image (in blocks) is given by `blocks_x` * `blocks_y`.
static void write_block(bitmap_pixel_rgb_t *pixels, uint32_t index_x, uint32_t index_y, uint32_t blocks_x, uint32_t blocks_y, const float *block)
{
	uint32_t bytes_per_row = blocks_x * 8;
	uint32_t base_offset = index_y * 8 * bytes_per_row;
	uint32_t col_offset = index_x * 8;

	for (uint32_t curr_y = 0; curr_y < 8; curr_y++)
	{
		uint32_t rowOffset = curr_y * bytes_per_row;

		for (uint32_t curr_x = 0; curr_x < 8; curr_x++)
		{
			uint32_t offset = base_offset + rowOffset + col_offset + curr_x;
			float component = roundf(block[(8 * curr_y) + curr_x]) + 128;

			bitmap_component_t clamped_component = (bitmap_component_t)MAX(0, MIN(component, 255));

			pixels[offset].r = clamped_component;
			pixels[offset].g = clamped_component;
			pixels[offset].b = clamped_component;
		}
	}
}

int decompress(const char* file_path, const uint32_t* quant_matrix, const char* output_path)
{
	float cosine_values[8][8];
	for (size_t m = 0; m < 8; m++) for (size_t p = 0; p < 8; p++)
		cosine_values[m][p] = cosf(M_PI * (2 * m + 1) * p / 16);

	// read blocks
	uint32_t blocks_x, blocks_y;
	FILE *input_file = fopen(file_path, "rb");

	if (!input_file)
		return -1;

	fread(&blocks_x, sizeof(uint32_t), 1, input_file);
	fread(&blocks_y, sizeof(uint32_t), 1, input_file);

	bitmap_pixel_rgb_t *pixels = malloc(sizeof(bitmap_pixel_rgb_t) * blocks_x * blocks_y * 64);

	// Walk all the blocks:
	for (uint32_t index_y = 0; index_y < blocks_y; index_y++)
	{
		for (uint32_t index_x = 0; index_x < blocks_x; index_x++)
		{
			int8_t input_block[64];
			int8_t un_zig_zagged[64];
			float de_quantized[64];
			float inverse_dct[64];

			fread(input_block, 1, 64, input_file);
			un_zig_zag(input_block, un_zig_zagged);
			dequantize(un_zig_zagged, quant_matrix, de_quantized);
			perform_inverse_dct(de_quantized, inverse_dct, cosine_values);

			write_block(pixels, index_x, index_y, blocks_x, blocks_y, inverse_dct);
		}
	}

	bitmap_parameters_t params = {
        .bottomUp = BITMAP_BOOL_TRUE,
        .widthPx = blocks_x * 8,
        .heightPx = blocks_y * 8,
        .colorDepth = BITMAP_COLOR_DEPTH_24,
        .compression = BITMAP_COMPRESSION_NONE,
        .dibHeaderFormat = BITMAP_DIB_HEADER_INFO,
        .colorSpace = BITMAP_COLOR_SPACE_RGB
    };

    // write the pixels back
    bitmap_error_t error = bitmapWritePixels(
        output_path,
        BITMAP_BOOL_TRUE,
        &params,
        (bitmap_pixel_t*)pixels
    );

	free(pixels);
	fclose(input_file);

	return 0;
}
