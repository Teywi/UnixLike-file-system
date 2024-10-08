/**
 * @file bmblock.c
 * @brief basic API for bitmap vectors
 *
 * @author Aurélien Soccard, Ludovic Mermod
 * @date 2022
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h> // for memset
#include <assert.h>
#include <inttypes.h>
#include "bmblock.h"
#include "error.h"
#include "unixv6fs.h"

#define ROUND_UP(_x, _y) ((((_x)+(_y)-1)/(_y))*(_y))

struct bmblock_array *bm_alloc(uint64_t min, uint64_t max)
{
    if (min > max) {//bounds included; size = max - min + 1
        return NULL;
    }
    size_t length = (max - min) / BITS_PER_VECTOR + 1; // number of blocks
    size_t alloc_size = ROUND_UP(sizeof(struct bmblock_array) + (length - 1) * sizeof(uint64_t), SECTOR_SIZE);
    // length - 1: since one is already present in the bm field: bm[0] is part of struct bmblock_array

    struct bmblock_array *bmblock = malloc(alloc_size);
    if (bmblock == NULL) {
        return NULL;
    }

    memset(bmblock, 0, alloc_size);
    bmblock->length = length;
    bmblock->max = max;
    bmblock->min = min;
    bmblock->cursor = UINT64_C(0);

    return bmblock;
}

int bm_get(struct bmblock_array *bmblock_array, uint64_t x)
{
    if ((x < bmblock_array->min) || (x > bmblock_array->max)) {
        return ERR_BAD_PARAMETER;
    }
    return (int) ((bmblock_array->bm[(x - bmblock_array->min) / BITS_PER_VECTOR]
                   >> ((x - bmblock_array->min) % BITS_PER_VECTOR)) & UINT64_C(1));
}

int bm_find_next(struct bmblock_array *bmblock_array)
{
    M_REQUIRE_NON_NULL(bmblock_array);

    uint64_t cursor = bmblock_array->cursor;

    for (size_t i = 0; i < bmblock_array->length; ++i) {
        uint64_t new_cursor = (cursor + i) % bmblock_array->length;
        bmblock_array->cursor = new_cursor;
        // we check first if the 64 are ones and jump to the next one if this is the case
        if (bmblock_array->bm[new_cursor] != UINT64_C(-1)) {
            for (uint64_t j = 0; j < BITS_PER_VECTOR; ++j) {
                uint64_t bit = new_cursor * BITS_PER_VECTOR + j + bmblock_array->min;
                if (bm_get(bmblock_array, bit) == 0) {
                    assert(bit <= bmblock_array->max && bit >= bmblock_array->min);
                    return (int) bit;
                }
            }
        }
    }

    return ERR_BITMAP_FULL;
}

void bm_set(struct bmblock_array *bmblock_array, uint64_t x)
{
    if (x <= bmblock_array->max && x >= bmblock_array->min) {
        bmblock_array->bm[(x - bmblock_array->min) / BITS_PER_VECTOR] |= (UINT64_C(1)
                << ((x - bmblock_array->min) % BITS_PER_VECTOR));
    }
}

void bm_clear(struct bmblock_array *bmblock_array, uint64_t x)
{
    if (x <= bmblock_array->max && x >= bmblock_array->min) {
        bmblock_array->bm[(x - bmblock_array->min) / BITS_PER_VECTOR] &= ~(UINT64_C(1)
                << ((x - bmblock_array->min) % BITS_PER_VECTOR));
    }
}

// tool functions
#define print_bit(value, position_mask) pps_printf("%c", value & position_mask ? '1' : '0')

void print_binary_byte(char value)
{
    /* Here we print REVERSE bit order (low bits first) as it makes more sense
     * for our own use.
     */
    print_bit(value, 0x01);
    print_bit(value, 0x02);
    print_bit(value, 0x04);
    print_bit(value, 0x08);
    print_bit(value, 0x10);
    print_bit(value, 0x20);
    print_bit(value, 0x40);
    print_bit(value, 0x80);
}

#define print_slice(value, slide)   print_binary_byte((char) ((value >> slide) & 0xff))
void println_binary_uint64_t(uint64_t value)
{
    /* Here we print REVERSE bit order (low bits first) as it makes more sense
     * for our own use.
     */
    print_slice(value,  0);
    pps_printf(" ");
    print_slice(value,  8);
    pps_printf(" ");
    print_slice(value, 16);
    pps_printf(" ");
    print_slice(value, 24);
    pps_printf(" ");
    print_slice(value, 32);
    pps_printf(" ");
    print_slice(value, 40);
    pps_printf(" ");
    print_slice(value, 48);
    pps_printf(" ");
    print_slice(value, 56);
    pps_printf("\n");
}

void bm_print(const char *name, struct bmblock_array *bm)
{
    if (name == NULL || bm ==NULL)
        return;
    pps_printf("**********BitMap Block %s START**********\n", name);
    pps_printf("length: %zu\n", bm->length);
    pps_printf("min: %" PRIu64 "\n", bm->min);
    pps_printf("max: %" PRIu64 "\n", bm->max);
    pps_printf("cursor: %" PRIu64 "\n", bm->cursor);
    pps_printf("content:\n");
    for (size_t i = 0; i < bm->length; ++i) {
        pps_printf("%2zu: ", i);
        println_binary_uint64_t(bm->bm[i]);
    }
    pps_printf("**********BitMap Block %s END************\n",name);
}
