#pragma once

#include <fuse.h>

/**
 * @file u6fs_fuse.h
 * @brief FUSE callbacks
 *
 * @author Edouard Bugnion
 * @date summer 2022
 */

/* *************************************************** *
 * TODO WEEK 08										   *
 * *************************************************** */
/**
 * @file u6fs_fuse.h
 * @brief Fills a stat struct with the attributes of a file
 *
 * @param path absolute path to the file
 * @param stbuf stat struct to fill
 * @return 0 on success, <0 on error
 */
int fs_getattr(const char *path, struct stat *stbuf);

/* *************************************************** *
 * TODO WEEK 08										   *
 * *************************************************** */
/**
 * @file u6fs_fuse.h
 * @brief Reads all the entries of a directory and output the result to a buffer using a given filler function
 *
 * @param path absolute path to the directory
 * @param buf buffer given to the filler function
 * @param filler function called for each entries, with the name of the entry and the buf parameter
 * @param offset ignored
 * @param fi fuse info -- ignored
 * @return 0 on success, <0 on error
 */
int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi);

/* *************************************************** *
 * TODO WEEK 08										   *
 * *************************************************** */
/**
 * @file u6fs_fuse.h
 * @brief reads at most size bytes from a file into a buffer
 * @param path absolute path to the file
 * @param buf buffer where read bytes will be written
 * @param size size in bytes of the buffer
 * @param offset read offset in the file
 * @param fi fuse info -- ignored
 * @return number of bytes read on success, <0 on error
 */
int fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi);

#ifdef CS212_TEST
// Sets the filesystem used by fs_* functions
// ONLY USED BY TEST FUNCTIONS
void fuse_set_fs(struct unix_filesystem *u);
#endif

/**
 * @brief mount the U6FS to an empty directory
 * @param u the filesystem (IN)
 * @param mountpoint the mount point in the host filesystem
 * @return 0 on success; the appropriate error code (<0) on error
 */
int u6fs_fuse_main(struct unix_filesystem *u, const char *mountpoint);

