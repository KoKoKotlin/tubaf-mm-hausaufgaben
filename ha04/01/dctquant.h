#ifndef DCTQUANT_H
#define DCTQUANT_H

#include <math.h>
#include <stdint.h>

// The default quantization matrix:
extern uint32_t default_quant_matrix[64];

// The ZigZag indices:
extern uint32_t zig_zag_index_matrix[64];

// The default quantization factor:
#define DEFAULT_QUANT_FACTOR 50

// Minimum and maximum:
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))

#define FOR_WRAPPER(p) for (size_t __i = 0; __i < 64; __i++) p

#define CLAMP(low, high, block) for (size_t ____i = 0; ____i < 64; ____i++) block[____i] = MIN(high, MAX(low, block[____i]))

// Alpha coefficients:
float alpha(uint8_t pq);

#endif
