#pragma once

/**
 * @file direntv6.h
 * @brief accessing the UNIX v6 filesystem -- directory layer
 *
 * @author Edouard Bugnion
 * @date summer 2022
 */

#include <stdint.h>
#include "unixv6fs.h"
#include "filev6.h"
#include "mount.h"

#pragma once


struct directory_reader {
    struct filev6 fv6;  // node of the directory
    struct direntv6 dirs[DIRENTRIES_PER_SECTOR];
    int cur;
    int last;
};

/* *************************************************** *
 * TODO WEEK 06										   *
 * *************************************************** */
/**
 * @brief opens a directory reader for the specified inode 'inr'
 * @param u the mounted filesystem
 * @param inr the inode -- which must point to an allocated directory
 * @param d the directory reader (OUT)
 * @return 0 on success; <0 on errror
 */
int direntv6_opendir(const struct unix_filesystem *u, uint16_t inr, struct directory_reader *d);

/* *************************************************** *
 * TODO WEEK 06										   *
 * *************************************************** */
/**
 * @brief return the next directory entry.
 * @param d the directory reader
 * @param name pointer to at least DIRENTMAX_LEN+1 bytes.  Filled in with the NULL-terminated string of the entry (OUT)
 * @param child_inr pointer to the inode number in the entry (OUT)
 * @return 1 on success;  0 if there are no more entries to read; <0 on error
 */
int direntv6_readdir(struct directory_reader *d, char *name, uint16_t *child_inr);

/* *************************************************** *
 * TODO WEEK 06										   *
 * *************************************************** */
/**
 * @brief debugging routine; print a subtree (note: recursive)
 * @param u a mounted filesystem
 * @param inr the root of the subtree
 * @param prefix the prefix to the subtree
 * @return 0 on success; <0 on error
 */
int direntv6_print_tree(const struct unix_filesystem *u, uint16_t inr, const char *prefix);

/* *************************************************** *
 * TODO WEEK 06										   *
 * *************************************************** */
/**
 * @brief get the inode number for the given path
 * @param u a mounted filesystem
 * @param inr the root of the subtree
 * @param entry the pathname relative to the subtree
 * @return inr on success; <0 on error
 */
int direntv6_dirlookup(const struct unix_filesystem *u, uint16_t inr, const char *entry);

/* *************************************************** *
 * TODO WEEK 12										   *
 * *************************************************** */
/**
 * @brief create a new direntv6 with the given name and given mode
 * @param u a mounted filesystem
 * @param entry the path of the new entry
 * @param mode the mode of the new inode
 * @return inr on success; <0 on error
 */
int direntv6_create(struct unix_filesystem *u, const char *entry, uint16_t mode);

/* *************************************************** *
 * TODO WEEK 21										   *
 * *************************************************** */
/**
 * @brief create a new direntv6 for a file
 * @param u a mounted filesystem
 * @param entry the path of the new entry
* @param mode the mode of the inode if the file is created (ignored otherwise)
 * @param buf the content of the file (to copy into the new entry)
 * @param size the size of the content
 * @return 0 on success; <0 on error
 */
int direntv6_addfile(struct unix_filesystem *u, const char *entry, uint16_t mode, char *buf, size_t size);
