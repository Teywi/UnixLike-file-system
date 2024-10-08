#pragma once

/**
 * @file u6fs_utils.h
 * @brief dump various elements of the UV6 filesystem
 *
 * @author Edouard Bugnion
 * @date summer 2022
 */

#include <stdio.h>
#include "mount.h"
#include "unixv6fs.h"


/**
 * @brief print to stdout the content of the superblock
 * @param u - the mounted filesytem
 * @return 0 on success, <0 on error
 */
int utils_print_superblock(const struct unix_filesystem *u);

/* *************************************************** *
 * TODO WEEK 05										   *
 * *************************************************** */
/**
 * @brief print to stdout the content of the an inode
 * @param u - the mounted filesytem
 * @param inr - the inode number
 * @return 0 on success, <0 on error
 */
int utils_print_inode(const struct inode *in);


/* *************************************************** *
 * TODO WEEK 05										   *
 * *************************************************** */
/**
 * @brief print to stdout the first sector of a file
 * @param u - the mounted filesytem
 * @param inr - the inode number of the file
 * @return 0 on success, <0 on error
 */
int utils_cat_first_sector(const struct unix_filesystem *u, uint16_t inr);

/* *************************************************** *
 * TODO WEEK 05										   *
 * *************************************************** */
/**
 * @brief print to stdout the SHA256 digest of the first UTILS_HASHED_LENGTH bytes of the file
 * @param u - the mounted filesystem
 * @param inr - the inode number of the file
 * @return 0 on success, <0 on error
 */
int utils_print_shafile(const struct unix_filesystem *u, uint16_t inr);


/* *************************************************** *
 * TODO WEEK 05										   *
 * *************************************************** */
/**
 * @brief print to stdout the SHA256 digest of all files, sorted by inode number
 * @param u - the mounted filesystem
 * @return 0 on success, <0 on error
 */
int utils_print_sha_allfiles(const struct unix_filesystem *u);

/* *************************************************** *
 * TODO WEEK 10										   *
 * *************************************************** */
/**
 * @brief print to stdout the inode and sector bitmaps
 * @param u - the mounted filesystem
 * @return 0 on success, <0 on error
 */
int utils_print_bitmaps(const struct unix_filesystem *u);
