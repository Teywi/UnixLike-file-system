/**
 * @file u6fs_utils.c
 * @brief Utilities (mostly dump) for UV6 filesystem
 * @author Aurélien Soccard / EB
 * @date 2022
 */

#include <string.h> 
#include <inttypes.h>
#include <openssl/sha.h>
#include "mount.h"
#include "sector.h"
#include "error.h"
#include "u6fs_utils.h"
#include "filev6.h"
#include "inode.h"
#include "bmblock.h"

int utils_print_superblock(const struct unix_filesystem *u){
    M_REQUIRE_NON_NULL(u);

    pps_printf("**********FS SUPERBLOCK START**********\n");
    pps_printf("%-20s: %" PRIu16 "\n", "s_isize", u->s.s_isize);
    pps_printf("%-20s: %" PRIu16 "\n", "s_fsize", u->s.s_fsize);
    pps_printf("%-20s: %" PRIu16 "\n", "s_fbmsize", u->s.s_fbmsize);
    pps_printf("%-20s: %" PRIu16 "\n", "s_ibmsize", u->s.s_ibmsize);
    pps_printf("%-20s: %" PRIu16 "\n", "s_inode_start", u->s.s_inode_start);
    pps_printf("%-20s: %" PRIu16 "\n", "s_block_start", u->s.s_block_start);
    pps_printf("%-20s: %" PRIu16 "\n", "s_fbm_start", u->s.s_fbm_start);
    pps_printf("%-20s: %" PRIu16 "\n", "s_ibm_start", u->s.s_ibm_start);
    pps_printf("%-20s: %" PRIu8 "\n", "s_flock", u->s.s_flock);
    pps_printf("%-20s: %" PRIu8 "\n", "s_ilock", u->s.s_ilock);
    pps_printf("%-20s: %" PRIu8 "\n", "s_fmod", u->s.s_fmod);
    pps_printf("%-20s: %" PRIu8 "\n", "s_ronly", u->s.s_ronly);
    pps_printf("%-20s: [%" PRIu16 "] %" PRIu16 "\n", "s_time", u->s.s_time[0], u->s.s_time[1]);
    pps_printf("**********FS SUPERBLOCK END**********\n");

    return ERR_NONE;
}

static void utils_print_SHA_buffer(unsigned char *buffer, size_t len){
    unsigned char sha[SHA256_DIGEST_LENGTH];
    SHA256(buffer, len, sha);
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i){
        pps_printf("%02x", sha[i]);
    }
    pps_printf("\n");
}

int utils_print_inode(const struct inode *inode){
    pps_printf("**********FS INODE START**********\n");
    if (inode == NULL){
        pps_printf("NULL ptr\n");
    }
    else{
        pps_printf("%s: %" PRIu16 "\n", "i_mode", inode->i_mode);
        pps_printf("%s: %" PRIu8 "\n", "i_nlink", inode->i_nlink);
        pps_printf("%s: %" PRIu8 "\n", "i_uid", inode->i_uid);
        pps_printf("%s: %" PRIu8 "\n", "i_gid", inode->i_gid);
        pps_printf("%s: %" PRIu8 "\n", "i_size0", inode->i_size0);
        pps_printf("%s: %" PRIu16 "\n", "i_size1", inode->i_size1);
        pps_printf("%s: %" PRIu32 "\n", "size", inode_getsize(inode));
    }
    pps_printf("**********FS INODE END************\n");

    return ERR_NONE;
}


int utils_cat_first_sector(const struct unix_filesystem *u, uint16_t inr){
    M_REQUIRE_NON_NULL(u);

    struct filev6 fv6;
    int read = filev6_open(u, inr, &fv6);
    if (read != ERR_NONE){
        pps_printf("filev6_open failed for inode #%d.\n", inr);
        return read;
    }

    pps_printf("\nPrinting inode #%d:\n", inr);
    read = utils_print_inode(&(fv6.i_node));
    if (read != ERR_NONE){
        return read;
    }

    if ((fv6.i_node).i_mode & IFDIR){
        pps_printf("which is a directory.\n");
    }
    else{
        pps_printf("the first sector of data of which contains:\n");

        uint8_t buf[SECTOR_SIZE] = {0};

        int num_bytes = filev6_readblock(&fv6, buf);
        if (num_bytes < ERR_NONE){
            return num_bytes;
        }
        for (size_t i = 0; i < num_bytes; i++){
            pps_printf("%c", buf[i]);
        }

        pps_printf("----\n");
    }

    return ERR_NONE;
}

int utils_print_shafile(const struct unix_filesystem *u, uint16_t inr){
    M_REQUIRE_NON_NULL(u);

    struct filev6 fv6 = {0};
    int read = filev6_open(u, inr, &fv6);
    if (read != ERR_NONE){
        return read;
    }
    pps_printf("SHA inode %" PRIu16 ": ", inr);

    if ((fv6.i_node).i_mode & IFDIR){
        pps_printf("DIR\n");
    }else{
        uint8_t buf[SECTOR_SIZE*INODES_PER_SECTOR+1];

        int num_bytes = -1;
        size_t l = 0;
        do{
            num_bytes = filev6_readblock(&fv6, &buf[l]);
            if (num_bytes < 0){
                return num_bytes;
            }
            
            l += num_bytes;
        }while(num_bytes > 0 && l < SECTOR_SIZE*INODES_PER_SECTOR);

        utils_print_SHA_buffer(buf, l);
    }

    return ERR_NONE;
}


int utils_print_sha_allfiles(const struct unix_filesystem *u){
    M_REQUIRE_NON_NULL(u);

    pps_printf("Listing inodes SHA\n");
    int read = 0;
    for (uint16_t i = 1; i < (u->s).s_isize * INODES_PER_SECTOR; i++){
        read = utils_print_shafile(u, i);
        if (read != ERR_NONE && read != ERR_UNALLOCATED_INODE){
            return read;
        }
    }

    return ERR_NONE;
}


int utils_print_bitmaps(const struct unix_filesystem *u){
    M_REQUIRE_NON_NULL(u);

    bm_print("INODES", u->ibm);
    bm_print("SECTORS", u->fbm);
    
    return ERR_NONE;
}

