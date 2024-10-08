#pragma once

/**
 * @file mount.h
 * @brief accessing the UNIX v6 filesystem -- core of the first set of assignments
 *
 * @author Edouard Bugnion
 * @date summer 2022
 */

#include <stdio.h>
#include "unixv6fs.h"
#include "bmblock.h"

struct unix_filesystem {
    FILE *f;
    struct superblock s;           /* copy of the superblock */
    struct bmblock_array *fbm;     /* block bitmap -- ignore before WEEK 10 */
    struct bmblock_array *ibm;     /* inode bitmap  -- ignore before WEEK 10 */
};


/* *************************************************** *
 * TODO WEEK 04: Implement							   *
 * TODO WEEK 10: Add bitmaps					   	   *
 * *************************************************** */
/**
 * @brief  mount a unix v6 filesystem
 * @param filename name of the unixv6 filesystem on the underlying disk (IN)
 * @param u the filesystem (OUT)
 * @return 0 on success; <0 on error
 */
int mountv6(const char *filename, struct unix_filesystem *u);


/* *************************************************** *
 * TODO WEEK 04: Implement							   *
 * TODO WEEK 10: Add bitmaps					   	   *
 * *************************************************** */
/**
 * @brief unmount the given filesystem
 * @param u - the mounted filesytem
 * @return 0 on success; <0 on error
 */
int umountv6(struct unix_filesystem *u);

/**
 * @brief create a new filesystem
 * @param num_blocks the total number of blocks (= max size of disk), in sectors
 * @param num_inodes the total number of inodes
 */
int mountv6_mkfs(const char *filename, uint16_t num_blocks, uint16_t num_inodes);

