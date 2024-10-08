#pragma once

/**
 * @file error.h
 * @brief file system error codes
 *
 * @author Edouard Bugnion
 * @date summer 2022
 */

#pragma once

#include <stdio.h> // for fprintf
#include <assert.h>

/**
 * @brief filesytem internal error codes.
 *
 */
#define ERR_NONE 0

enum error_codes {
    ERR_FIRST = -128, // not an actual error but to set the first error number
    ERR_INVALID_COMMAND,
    ERR_NOMEM,
    ERR_IO,
    ERR_BAD_BOOT_SECTOR,
    ERR_INODE_OUT_OF_RANGE,
    ERR_FILENAME_TOO_LONG,
    ERR_INVALID_DIRECTORY_INODE,
    ERR_UNALLOCATED_INODE,
    ERR_FILENAME_ALREADY_EXISTS,
    ERR_BITMAP_FULL,
    ERR_FILE_TOO_LARGE,
    ERR_OFFSET_OUT_OF_RANGE,
    ERR_BAD_PARAMETER,
    ERR_NO_SUCH_FILE,
    ERR_LAST // not an actual error but to have e.g. the total number of errors
};

/*
 * Helpers (macros)
 */

// example: ASSERT(x>0);

#define ASSERT assert

#ifdef DEBUG
#define debug_printf(fmt, ...) \
    fprintf(stderr, fmt, __VA_ARGS__)
#else
#define debug_printf(fmt, ...) \
    do {} while(0)
#endif

#define M_REQUIRE_NON_NULL(arg) \
    do { \
        if (arg == NULL) { \
            debug_printf("ERROR: parameter %s is NULL when calling  %s() (defined in %s)\n", \
                        #arg, __func__, __FILE__); \
            return ERR_BAD_PARAMETER; \
        } \
    } while(0)


#define pps_printf printf

/**
* @brief filesystem internal error messages. defined in error.c
*
*/
extern const char * const ERR_MESSAGES[];
