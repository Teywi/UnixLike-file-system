#include <string.h>
#include "error.h"
#include "unixv6fs.h"
#include "filev6.h"
#include "inode.h"
#include "sector.h"

#define END_OF_FILE 0


int filev6_open(const struct unix_filesystem *u, uint16_t inr, struct filev6 *fv6){
    M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(fv6);

    int read = inode_read(u, inr, &(fv6->i_node));
    if (read != ERR_NONE){
        return read;
    }
    fv6->i_number = inr;
    fv6->offset = 0;
    fv6->u = u;

    return ERR_NONE;
}


int filev6_readblock(struct filev6 *fv6, void *buf){
    M_REQUIRE_NON_NULL(fv6);
    M_REQUIRE_NON_NULL(buf);

    uint32_t file_size = inode_getsize(&(fv6->i_node));

    if(fv6->offset == file_size){
        return END_OF_FILE;
    }

    int sector_id = inode_findsector(fv6->u, &(fv6->i_node), (fv6->offset)/SECTOR_SIZE); 
    if(sector_id < END_OF_FILE){
        return sector_id;
    }
    int read = sector_read((fv6->u)->f, sector_id, buf);
    if(read != ERR_NONE){
        return read;
    }

    uint32_t bytes_to_read = ((fv6->offset + SECTOR_SIZE) <= file_size) ? SECTOR_SIZE : (file_size - fv6->offset);
    fv6->offset += bytes_to_read;
    
    return (int)bytes_to_read;
}


int filev6_lseek(struct filev6 *fv6, int32_t offset){
    M_REQUIRE_NON_NULL(fv6);

    size_t file_size = inode_getsize(&(fv6->i_node));

    if(offset == file_size){
        fv6->offset = offset;
        return ERR_NONE;
    } 

    if(offset%SECTOR_SIZE != 0){return ERR_BAD_PARAMETER;}
    
    if(offset < 0 || offset > file_size){return ERR_OFFSET_OUT_OF_RANGE;}

    fv6->offset = offset;

    return ERR_NONE;
}


int filev6_create(struct unix_filesystem *u, uint16_t mode, struct filev6 *fv6){
    M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(fv6);

    int inr = inode_alloc(u);
    if(inr < ROOT_INUMBER){
        return inr;
    }

    struct inode inode;
    memset(&inode, 0, sizeof(inode));
    inode.i_mode = IALLOC | mode;

    int write = inode_write(u, inr, &inode);
    if(write != ERR_NONE){
        return write;
    }

    fv6->u = u;
    fv6->i_number = inr;
    fv6->i_node = inode;
    fv6->offset = 0;

    return ERR_NONE;
}


int filev6_writesector(struct filev6 *fv6, const void *buf, size_t len, size_t left_to_write, size_t bytes_writen, size_t size_file){ //len should be either 512 to write a full sector or less
    M_REQUIRE_NON_NULL(fv6);
    M_REQUIRE_NON_NULL(buf); 

    size_t nb_bytes = size_file%SECTOR_SIZE; 
    if(nb_bytes != 0){ //we need to fill a sector
        nb_bytes = SECTOR_SIZE - nb_bytes;
        nb_bytes = (nb_bytes < len) ? nb_bytes : len;

        int offset_sector = size_file/SECTOR_SIZE;
        uint8_t sector_to_fill[SECTOR_SIZE+1] = {0};
        int read = sector_read((fv6->u)->f, (fv6->i_node).i_addr[offset_sector], sector_to_fill);
        
        if(read != ERR_NONE){
            return read;
        }
        memcpy(&(sector_to_fill[size_file%SECTOR_SIZE]), buf, nb_bytes);
        
        int write = sector_write((fv6->u)->f, (fv6->i_node).i_addr[offset_sector], sector_to_fill);
        if(write != ERR_NONE){
            return write;
        }
        bytes_writen = nb_bytes;
        left_to_write -= nb_bytes;
        size_file += nb_bytes;
    }
    else{
        nb_bytes = (left_to_write >= SECTOR_SIZE) ? SECTOR_SIZE : left_to_write;

        int added_sector_id = bm_find_next((fv6->u)->fbm);
        if(added_sector_id < (fv6->u)->s.s_block_start){       //then not a valid sector
            return added_sector_id;
        }
        bm_set((fv6->u)->fbm, added_sector_id); 

        uint8_t sector_to_write[SECTOR_SIZE] = {0};
        memcpy(sector_to_write, &buf[bytes_writen], nb_bytes);
        int write = sector_write((fv6->u)->f, added_sector_id, sector_to_write);
        if(write != ERR_NONE){
            return write;
        }

        int offset_sector = size_file/SECTOR_SIZE;
        (fv6->i_node).i_addr[offset_sector] = added_sector_id;
    }

    return nb_bytes;
}


int filev6_writebytes(struct filev6 *fv6, const void *buf, size_t len){
    M_REQUIRE_NON_NULL(fv6);
    M_REQUIRE_NON_NULL(buf);

    size_t left_to_write = len;
    size_t bytes_writen = 0;

    uint32_t size_file = inode_getsize(&(fv6->i_node));
    if(size_file + len > (ADDR_SMALL_LENGTH-1)*ADDRESSES_PER_SECTOR*SECTOR_SIZE){ //file too large to fit
        return ERR_FILE_TOO_LARGE;
    }
    if(size_file + len >= ADDR_SMALL_LENGTH*SECTOR_SIZE){
        return ERR_FILE_TOO_LARGE; //we need to return an error because our function doesn't treat this case
    }

    size_t nb_bytes = 0; 
    while(left_to_write != 0){
        nb_bytes = filev6_writesector(fv6, buf, len, left_to_write, bytes_writen, size_file);
        if(nb_bytes < 0){
            return nb_bytes;
        }

        bytes_writen += nb_bytes;  //we shift the pointer of buf by this value
        left_to_write -= nb_bytes;  
        size_file += nb_bytes; //we update the size of the file to change the size of the inode in the end
    }

    int set_size = inode_setsize(&(fv6->i_node), size_file);
    if(set_size != ERR_NONE){
        return set_size;
    }

    int write_inode = inode_write(fv6->u, fv6->i_number, &(fv6->i_node));
    if(write_inode != ERR_NONE){
        return write_inode;
    }

    return ERR_NONE;
}
