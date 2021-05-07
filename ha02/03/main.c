#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <emmintrin.h>
#include <time.h>

#include "lib/bitmap.h"

struct MandelSpec {
    //image specs
    unsigned int width;
    unsigned int height;
    unsigned int depth;

    //mandel specs
    float xlim[2];
    float ylim[2];
    unsigned int iterations;
};

void mandel_basic(bitmap_pixel_rgb_t *image, const struct MandelSpec *s){
    clock_t start = clock();
    //x and y range in the complex plane
    float xdiff = s->xlim[1] - s->xlim[0];
    float ydiff = s->ylim[1] - s->ylim[0];

    float iter_scale = 1.0f / s->iterations;
    float depth_scale = s->depth - 1;

    //iterate over all pixels in the image
    for (int y = 0; y < s->height; y++) {
        for (int x = 0; x < s->width; x++) {

            //get the real and imaginary part of c scaled by the image width and height and offset
            float cr = x * xdiff / s->width  + s->xlim[0];
            float ci = y * ydiff / s->height + s->ylim[0];

            //this will be z_n, here the initial term z0
            float zr = cr;
            float zi = ci;

            //iteration steps
            float mk = 0.0f;

            while (++mk < s->iterations) {

                //z_n+1 = z_n² + c
                float zr1 = zr * zr - zi * zi + cr;
                float zi1 = zr * zi + zr * zi + ci;

                //save z_n+1
                zr = zr1;
                zi = zi1;

                mk += 1.0f;

                //bounded and therefore part of the mandelbrot set, if |z_n+1| > 2
                if (zr * zr + zi * zi >= 4.0f)
                    break;
            }

            //mk is between 0 and s->iterations
            //scale it to fit between 0 and 1
            mk *= iter_scale;

            //damping for nicer color gradients
            mk = sqrtf(mk);

            //scale up with color depth of image
            mk *= depth_scale;

            int pixelValue = mk;
            image[y * s->width + x].r = pixelValue;
            image[y * s->width + x].g = pixelValue;
            image[y * s->width + x].b = pixelValue;
        }
    }
    printf("Mandel Basic: %.6fms\n", (double)(clock() - start) / (CLOCKS_PER_SEC / 1000));
}

inline static __m128 load_array(float* arr)
{
    __m128 v_sse = _mm_loadu_ps(arr);    
    return v_sse;
}

inline static size_t make_div_by_four(size_t s)
{
    return s % 4 == 0 ? s : s + (4 - s % 4);
}

inline static __m128 coordinate_transformation(__m128 coords, __m128 diff, __m128 range, __m128 lim) {
    __m128 scaled_coords = _mm_mul_ps(coords, diff);
    __m128 normalized_coords = _mm_div_ps(scaled_coords, range);
    return _mm_add_ps(normalized_coords, lim);
}

void print_mm(__m128 m) {
    float temp[4];
    _mm_storeu_ps(temp, m);

    for(size_t i = 0; i < 4; i++) printf("%f ", temp[i]);
    printf(" ");
}

void mandel_sse2(bitmap_pixel_rgb_t *image, const struct MandelSpec *s)
{   
    clock_t start = clock();
    float xdiff = s->xlim[1] - s->xlim[0];
    float ydiff = s->ylim[1] - s->ylim[0];

    float iter_scale = 1.0f / s->iterations;
    float depth_scale = s->depth - 1;

    __m128 xdiff_sse  = _mm_set1_ps(xdiff);
    __m128 ydiff_sse  = _mm_set1_ps(ydiff);
    __m128 xlim_sse   = _mm_set1_ps(s->xlim[0]);
    __m128 ylim_sse   = _mm_set1_ps(s->ylim[0]);
    __m128 width_sse  = _mm_set1_ps((float)s->width);
    __m128 height_sse = _mm_set1_ps((float)s->height);
    __m128 ones       = _mm_set1_ps(1.0f);
    __m128 fours      = _mm_set1_ps(4.0f);

    float *out_buffer = NULL;
    posix_memalign((void**)&out_buffer, 16, sizeof(float) * make_div_by_four(s->width) * s->height);

    //iterate over all pixels in the image
    for(float y = 0; y < s->height; y++) {
        for(float x = 0; x < make_div_by_four(s->width) / 4; x++) {
            float xarr[] = { x * 4, x * 4 + 1, x * 4 + 2, x * 4 + 3 };

            __m128 xs = load_array(xarr);
            __m128 ys = _mm_set1_ps(y);

            __m128 real_part = coordinate_transformation(xs, xdiff_sse, width_sse,  xlim_sse);
            __m128 imag_part = coordinate_transformation(ys, ydiff_sse, height_sse, ylim_sse);
            
            //this will be z_n, here the initial term z0
            __m128 z_real = real_part;
            __m128 z_imag = imag_part;

            __m128 bounded_info = _mm_set1_ps(-NAN);
            __m128 bounded      = ones;

            //iteration steps
            size_t mk = 0;
            __m128 mks = _mm_set1_ps(0.0f);

            while (++mk < s->iterations && _mm_movemask_ps(bounded_info)) {
                // z_n+1 = z_n² + c = (a + bi)^2 + c
                // Re(z_n+1) = a^2 - b^2 + cr
                // Im(z_n+1) = 2 * a * b + ci

                __m128 z_real_2 = _mm_mul_ps(z_real, z_real);
                __m128 z_imag_2 = _mm_mul_ps(z_imag, z_imag);
                __m128 z_mixed  = _mm_mul_ps(z_imag, z_real);
                
                __m128 new_z_real = _mm_add_ps(_mm_sub_ps(z_real_2, z_imag_2), real_part);
                __m128 new_z_imag = _mm_add_ps(_mm_add_ps(z_mixed, z_mixed), imag_part);
                
                __m128 new_z_real_diff = _mm_sub_ps(new_z_real, z_real);
                __m128 new_z_imag_diff = _mm_sub_ps(new_z_imag, z_imag);
                
                //save z_n+1
                z_real = _mm_add_ps(z_real, _mm_mul_ps(new_z_real_diff, bounded));
                z_imag = _mm_add_ps(z_imag, _mm_mul_ps(new_z_imag_diff, bounded));


                z_real_2 = _mm_mul_ps(z_real, z_real);
                z_imag_2 = _mm_mul_ps(z_imag, z_imag);
                
                //bounded and therefore part of the mandelbrot set, if |z_n+1| > 2
                __m128 z_abs = _mm_add_ps(z_real_2, z_imag_2);
                bounded_info = _mm_cmplt_ps(z_abs, fours);
                bounded = _mm_and_ps(bounded_info, ones);

                mks = _mm_add_ps(mks, bounded);
            }

            __m128 iter_scale_sse  = _mm_set1_ps(iter_scale);
            __m128 depth_scale_sse = _mm_set1_ps(depth_scale);

            //mk is between 0 and s->iterations
            //scale it to fit between 0 and 1
            mks = _mm_mul_ps(mks, iter_scale_sse);

            //damping for nicer color gradients
            mks = _mm_sqrt_ps(mks);

            //scale up with color depth of image
            mks = _mm_mul_ps(mks, depth_scale_sse);

            _mm_store_ps(out_buffer + (size_t)y * s->width + (size_t)x * 4, mks);
        }
    }

    for(size_t y = 0; y < s->height; y++) {
        for (size_t x = 0; x < s->width; x++) {
            image[y * s->width + x].r = (uint8_t)out_buffer[y * s->width + x];
            image[y * s->width + x].g = (uint8_t)out_buffer[y * s->width + x];
            image[y * s->width + x].b = (uint8_t)out_buffer[y * s->width + x];
        }
    }

    free(out_buffer);
    printf("Mandel SSE: %.6fms\n", (double)(clock() - start) / (CLOCKS_PER_SEC / 1000));
}

void mandel_sse24(bitmap_pixel_rgb_t *image, const struct MandelSpec *s)
{   
    clock_t start = clock();
    float xdiff = s->xlim[1] - s->xlim[0];
    float ydiff = s->ylim[1] - s->ylim[0];

    float iter_scale = 1.0f / s->iterations;
    float depth_scale = s->depth - 1;

    __m128 xdiff_sse  = _mm_set1_ps(xdiff);
    __m128 ydiff_sse  = _mm_set1_ps(ydiff);
    __m128 xlim_sse   = _mm_set1_ps(s->xlim[0]);
    __m128 ylim_sse   = _mm_set1_ps(s->ylim[0]);
    __m128 width_sse  = _mm_set1_ps((float)s->width);
    __m128 height_sse = _mm_set1_ps((float)s->height);
    __m128 ones       = _mm_set1_ps(1.0f);
    __m128 fours      = _mm_set1_ps(4.0f);

    float *out_buffer = NULL;
    posix_memalign((void**)&out_buffer, 16, sizeof(float) * make_div_by_four(s->width) * s->height);

    //iterate over all pixels in the image
    for(float y = 0; y < s->height; y++) {
        for(float x = 0; x < make_div_by_four(s->width) / 4; x++) {
            float xarr[] = { x * 4, x * 4 + 1, x * 4 + 2, x * 4 + 3 };

            __m128 xs = load_array(xarr);
            __m128 ys = _mm_set1_ps(y);

            __m128 real_part = coordinate_transformation(xs, xdiff_sse, width_sse,  xlim_sse);
            __m128 imag_part = coordinate_transformation(ys, ydiff_sse, height_sse, ylim_sse);
            
            //this will be z_n, here the initial term z0
            __m128 z_real = real_part;
            __m128 z_imag = imag_part;

            __m128 bounded_info = _mm_set1_ps(-NAN);
            __m128 bounded      = ones;

            //iteration steps
            size_t mk = 1;
            __m128 mks = _mm_set1_ps(0.0f);

            while (mk < s->iterations && _mm_movemask_ps(bounded_info)) {
                // z_n+1 = z_n² + c = (a + bi)^2 + c
                // Re(z_n+1) = a^2 - b^2 + cr
                // Im(z_n+1) = 2 * a * b + ci

                __m128 z_real_2 = _mm_mul_ps(z_real, z_real);
                __m128 z_imag_2 = _mm_mul_ps(z_imag, z_imag);
                __m128 z_mixed  = _mm_mul_ps(z_imag, z_real);
                
                __m128 new_z_real = _mm_add_ps(_mm_sub_ps(z_real_2, z_imag_2), real_part);
                __m128 new_z_imag = _mm_add_ps(_mm_add_ps(z_mixed, z_mixed), imag_part);
                
                __m128 new_z_real_diff = _mm_sub_ps(new_z_real, z_real);
                __m128 new_z_imag_diff = _mm_sub_ps(new_z_imag, z_imag);
                
                //save z_n+1
                z_real = _mm_add_ps(z_real, _mm_mul_ps(new_z_real_diff, bounded));
                z_imag = _mm_add_ps(z_imag, _mm_mul_ps(new_z_imag_diff, bounded));


                z_real_2 = _mm_mul_ps(z_real, z_real);
                z_imag_2 = _mm_mul_ps(z_imag, z_imag);
                
                //bounded and therefore part of the mandelbrot set, if |z_n+1| > 2
                __m128 z_abs = _mm_add_ps(z_real_2, z_imag_2);
                bounded_info = _mm_cmplt_ps(z_abs, fours);
                bounded = _mm_and_ps(bounded_info, ones);

                mks = _mm_add_ps(mks, bounded);
                mk+=4; //4 steps each loop
            }

            __m128 iter_scale_sse  = _mm_set1_ps(iter_scale);
            __m128 depth_scale_sse = _mm_set1_ps(depth_scale);

            //mk is between 0 and s->iterations
            //scale it to fit between 0 and 1
            mks = _mm_mul_ps(mks, iter_scale_sse);

            //damping for nicer color gradients
            mks = _mm_sqrt_ps(mks);

            //scale up with color depth of image
            mks = _mm_mul_ps(mks, depth_scale_sse);

            _mm_store_ps(out_buffer + (size_t)y * s->width + (size_t)x * 4, mks);
        }
    }

    for(size_t y = 0; y < s->height; y++) {
        for (size_t x = 0; x < s->width; x++) {
            image[y * s->width + x].r = (uint8_t)out_buffer[y * s->width + x];
            image[y * s->width + x].g = (uint8_t)out_buffer[y * s->width + x];
            image[y * s->width + x].b = (uint8_t)out_buffer[y * s->width + x];
        }
    }

    free(out_buffer);
    printf("Mandel SSE x4: %.6fms\n", (double)(clock() - start) / (CLOCKS_PER_SEC / 1000));
}

int main(int argc, char *argv[]){

    //default params
    unsigned int width = 1920;
    float x = -0.5;
    float y = 0.0;
    float r = 2;

    if(argc < 2){
        printf("Creating image with default parameters...\n");
        printf("Possible usage:\n");
        printf("$>./mandel 1920\t\t\t//image width\n");
        printf("$>./mandel 1920 -0.5 0.0\t//image width, middle point coordindate\n");
        printf("$>./mandel 1920 -0.5 0.0 2.0\t//image width, middle point coordindate, range around middle point\n");
    }

    if(argc == 2){
        width = atoi(argv[1]);
    }

    if(argc == 4){
        width = atoi(argv[1]);
        x = atof(argv[2]);
        y = atof(argv[3]);
    }

    if(argc == 5){
        width = atoi(argv[1]);
        x = atof(argv[2]);
        y = atof(argv[3]);
        r = atof(argv[4]);
    }

    //limits of complex plane to draw
    float xMin = x - r;
    float xMax = x + r;
    float yMin = y - 0.75 * r;
    float yMax = y + 0.75 * r;

    //get height from aspect ratio
    unsigned int height = (unsigned int)(width * 0.75);

    struct MandelSpec spec = {
        .width = width,
        .height = height,
        .depth = 256,
        .xlim = {xMin, xMax},
        .ylim = {yMin, yMax},
        .iterations = 256
    };

    bitmap_pixel_t *image = malloc(spec.width * spec.height * sizeof(bitmap_pixel_t));


    mandel_basic((bitmap_pixel_rgb_t*)image, &spec);

    mandel_sse2((bitmap_pixel_rgb_t*)image, &spec);

    mandel_sse24((bitmap_pixel_rgb_t*)image, &spec);


    bitmap_parameters_t params;
    memset(&params, 0, sizeof(bitmap_parameters_t));
    params.bottomUp = BITMAP_BOOL_TRUE;
    params.widthPx = width;
    params.heightPx = height;
    params.colorDepth = BITMAP_COLOR_DEPTH_32;
    params.compression = BITMAP_COMPRESSION_NONE;
    params.dibHeaderFormat = BITMAP_DIB_HEADER_INFO;

    char filename[64];
    snprintf(filename, 64, "mandel_%d_%f_%f_%f.bmp", width, x, y, r);

    switch(bitmapWritePixels(filename, BITMAP_BOOL_TRUE, &params, image)){
        case BITMAP_ERROR_SUCCESS:
            printf("Saved as %s.\n", filename);
            break;
        case BITMAP_ERROR_INVALID_PATH:
            printf("Invalid path.\n");
            break;
        case BITMAP_ERROR_INVALID_FILE_FORMAT:
            printf("Invalid file format.\n");
            break;
        case BITMAP_ERROR_IO:
            printf("IO Error.\n");
            break;
        case BITMAP_ERROR_MEMORY:
            printf("Memory error.\n");
            break;
        case BITMAP_ERROR_FILE_EXISTS:
            printf("File exists, override not allowed.\n");
            break;
    };

    return 0;
}