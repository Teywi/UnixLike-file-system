/**
 * @file mount.c
 * @brief mounts the filesystem
 *
 * @author Ludovic Mermod / Aurélien Soccard / Edouard Bugnion
 * @date 2022
 */

#include <string.h>
#include <inttypes.h>
#include <stdlib.h>

#include "error.h"
#include "mount.h"
#include "sector.h"
#include "inode.h"
#include "bmblock.h"

int mountv6(const char *filename, struct unix_filesystem *u){
    M_REQUIRE_NON_NULL(filename);
    M_REQUIRE_NON_NULL(u);
    
    memset(u, 0, sizeof(*u));
    u->f = fopen(filename, "rb+");
    if(u->f == NULL){
        return ERR_IO;
    }

    uint8_t data[SECTOR_SIZE] = {0};
    int read = sector_read(u->f, BOOTBLOCK_SECTOR, data);
    if(read != ERR_NONE){
        fclose(u->f);
        return read;
    }

    if (data[BOOTBLOCK_MAGIC_NUM_OFFSET] != BOOTBLOCK_MAGIC_NUM){
        fclose(u->f);
        return ERR_BAD_BOOT_SECTOR;
    }
    
    int read2 = sector_read(u->f, SUPERBLOCK_SECTOR, &u->s);
    if(read2 != ERR_NONE){
        fclose(u->f);
        return read2;
    }

    u->ibm = bm_alloc(ROOT_INUMBER, u->s.s_isize*INODES_PER_SECTOR + ROOT_INUMBER - 1); //ROOT_INUMBER - 1 = 0, but we still put it in case we change the value of ROOT_INUMBER
    if(u->ibm == NULL){
        fclose(u->f);
        return ERR_NOMEM;
    }

    u->fbm = bm_alloc(u->s.s_block_start, u->s.s_fsize);
    if(u->fbm == NULL){
        fclose(u->f);
        free(u->ibm);
        u->ibm = NULL;
        return ERR_NOMEM;
    }
	
    for(uint16_t inr = ROOT_INUMBER; inr < (u->s).s_isize*INODES_PER_SECTOR; inr++){
		struct inode inode;
		int output_scan = inode_read(u, inr, &inode); 
		if (output_scan != ERR_UNALLOCATED_INODE){
            bm_set(u->ibm, inr);

            int32_t size_file = inode_getsize(&inode);
            int sector_nbr;
            int32_t offset = 0;
            while((sector_nbr = inode_findsector(u, &inode, offset)) > 0 ){
                if((size_file > ADDR_SMALL_LENGTH*SECTOR_SIZE) && (offset%ADDRESSES_PER_SECTOR == 0)){
                    bm_set(u->fbm, inode.i_addr[offset/ADDRESSES_PER_SECTOR]);
                }
                bm_set(u->fbm, sector_nbr);
                offset++;
            }
		}
	}
	
    return ERR_NONE;
}


int umountv6(struct unix_filesystem *u){   
    M_REQUIRE_NON_NULL(u);

    if(u->f == NULL){
        return ERR_IO;
    }

    free(u->ibm);
    u->ibm = NULL;

    free(u->fbm);
    u->fbm = NULL;

    int success = fclose(u->f);
    if(success){
        return ERR_IO;
    }
    return ERR_NONE;
}

