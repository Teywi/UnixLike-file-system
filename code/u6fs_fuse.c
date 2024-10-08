/**
 * @file u6fs_fuse.c
 * @brief interface to FUSE (Filesystem in Userspace)
 *
 * @date 2022
 * @author Édouard Bugnion, Ludovic Mermod
 *  Inspired from hello.c from:
 *    FUSE: Filesystem in Userspace
 *    Copyright (C) 2001-2007  Miklos Szeredi <miklos@szeredi.hu>
 *
 *  This program can be distributed under the terms of the GNU GPL.
 */

#define FUSE_USE_VERSION 26

#include <fuse.h>
#include <string.h>
#include <fcntl.h>
#include <math.h> // ???

#include <stdlib.h> // for exit()
#include "mount.h"
#include "error.h"
#include "inode.h"
#include "direntv6.h"
#include "u6fs_utils.h"
#include "u6fs_fuse.h"
#include "util.h"

#define MAX_BUF_SIZE 65536
#define SUCCESS 1

static struct unix_filesystem* theFS = NULL; // usefull for tests

int fs_getattr(const char *path, struct stat *stbuf){
    M_REQUIRE_NON_NULL(path);
    M_REQUIRE_NON_NULL(stbuf);
    M_REQUIRE_NON_NULL(theFS);

    int inr = direntv6_dirlookup(theFS, ROOT_INUMBER, path);
    if(inr < 0){
        return inr;
    }

    struct inode i;
    int read = inode_read(theFS, inr, &i);
    if(read != ERR_NONE){
        return read;
    }

    stbuf->st_mode = S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH;
    stbuf->st_mode |= (i.i_mode & IFDIR) ? __S_IFDIR : __S_IFREG;
    stbuf->st_size = inode_getsize(&i);
    stbuf->st_ino = inr;
    stbuf->st_blksize = SECTOR_SIZE;
    stbuf->st_blocks = ceil(1.0*stbuf->st_size / SECTOR_SIZE); // Use math.h
    stbuf->st_uid = i.i_uid;
    stbuf->st_gid = i.i_gid;
    stbuf->st_nlink = i.i_nlink;

    return ERR_NONE;
}

// Insert directory entries into the directory structure, which is also passed to it as buf
int fs_readdir(const char *path, void *buf, fuse_fill_dir_t filler, off_t offset _unused, struct fuse_file_info *fi){
    M_REQUIRE_NON_NULL(path);
    M_REQUIRE_NON_NULL(buf);
    M_REQUIRE_NON_NULL(fi);
    M_REQUIRE_NON_NULL(filler);

    int inr = direntv6_dirlookup(theFS, ROOT_INUMBER, path);
    if (inr < 0){
        return inr;
    }

    struct directory_reader d;

    int check = direntv6_opendir(theFS, inr, &d); 
    if(check != ERR_NONE){
        return check;
    }
    
    uint16_t child_inr = 0; 

    struct stat stdbuf = {0};

    char name[DIRENT_MAXLEN+1];

    check = filler(buf, ".", NULL, 0);
    if(check != ERR_NONE){
        return ERR_NOMEM;
    }

    check = filler(buf, "..", NULL, 0);
    if(check != ERR_NONE){
        return ERR_NOMEM;
    }

    int cont_read = 0;
    while((cont_read = direntv6_readdir(&d, name, &child_inr)) == SUCCESS){
        if(cont_read <= 0){
            return cont_read;
        } 

        check = filler(buf, name, NULL, 0);
        if(check != ERR_NONE){
            return ERR_NOMEM;
        }
    }

    return ERR_NONE;
}


int fs_read(const char *path, char *buf, size_t size, off_t offset, struct fuse_file_info *fi){
    M_REQUIRE_NON_NULL(path);
    M_REQUIRE_NON_NULL(buf);
    M_REQUIRE_NON_NULL(fi);
    M_REQUIRE_NON_NULL(theFS);

    struct filev6 fv6;;
    int inr = direntv6_dirlookup(theFS, ROOT_INUMBER, path);
    if(inr < 0){
        return inr;
    }

    int read = filev6_open(theFS, inr, &fv6);
    if(read != ERR_NONE){
        return read;
    }

    read = filev6_lseek(&fv6, offset);
    if(read != ERR_NONE){
        return read;
    }

    int num_bytes = -1;
    size_t l = 0;
    do{
        num_bytes = filev6_readblock(&fv6, buf);
        if (num_bytes < 0){
            return num_bytes;
        }
        l += num_bytes;
        buf += num_bytes;

    }while(num_bytes > 0 && l < size);

    return l;
}


static struct fuse_operations available_ops = {
    .getattr = fs_getattr,
    .readdir = fs_readdir,
    .read    = fs_read,
};

int u6fs_fuse_main(struct unix_filesystem *u, const char *mountpoint)
{
    M_REQUIRE_NON_NULL(mountpoint);

    theFS = u;  // /!\ GLOBAL ASSIGNMENT
    const char *argv[] = {
        "u6fs",
        "-s",               // * `-s` : single threaded operation
        "-f",              // foreground operation (no fork).  alternative "-d" for more debug messages
        "-odirect_io",      //  no caching in the kernel.
#ifdef DEBUG
        "-d",
#endif
        //  "-ononempty",    // unused
        mountpoint
    };
    // very ugly trick when a cast is required to avoid a warning
    void *argv_alias = argv;

    utils_print_superblock(theFS);
    int ret = fuse_main(sizeof(argv) / sizeof(char *), argv_alias, &available_ops, NULL);
    theFS = NULL; // /!\ GLOBAL ASSIGNMENT
    return ret;
}

#ifdef CS212_TEST
void fuse_set_fs(struct unix_filesystem *u)
{
    theFS = u;
}
#endif
