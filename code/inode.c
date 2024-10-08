#include <inttypes.h>
#include <string.h>
#include "error.h"
#include "unixv6fs.h"
#include "sector.h"
#include "inode.h"
#include "bmblock.h"

#define NB_INDIR_SECTORS (ADDR_SMALL_LENGTH-1)


int inode_read(const struct unix_filesystem *u, uint16_t inr, struct inode *inode){
	M_REQUIRE_NON_NULL(u);
	M_REQUIRE_NON_NULL(inode);

	if(inr < ROOT_INUMBER || inr >= (u->s).s_isize*INODES_PER_SECTOR){ 
		return ERR_INODE_OUT_OF_RANGE; 
	}

	uint32_t num_sector = (u->s).s_inode_start + inr/INODES_PER_SECTOR; 
	uint16_t place_in_sector = inr%INODES_PER_SECTOR;
	struct inode_sector inodes_in_sector;
	int read_output = sector_read(u->f, num_sector, inodes_in_sector.inodes);
	if(read_output != ERR_NONE){
		return read_output;
	}
	
	memcpy(inode, &(inodes_in_sector.inodes[place_in_sector]), sizeof(*inode)); 
	if (!(inode->i_mode & IALLOC)){ 
		return ERR_UNALLOCATED_INODE; 
	}
	return ERR_NONE;
	
}

int inode_scan_print(const struct unix_filesystem *u){
	M_REQUIRE_NON_NULL(u);

	uint16_t inr;
	for(inr = ROOT_INUMBER; inr < (u->s).s_isize*INODES_PER_SECTOR; inr++){
		struct inode inode;
		int output_scan = inode_read(u, inr, &inode); 
		if (output_scan != ERR_UNALLOCATED_INODE){
			if(output_scan != ERR_NONE){
				return output_scan;
			}else{
				int32_t size_inode = inode_getsize(&inode);
				pps_printf("inode %" PRIu16 " (%s) len %" PRIu32 "\n", inr, inode.i_mode & IFDIR ? SHORT_DIR_NAME : SHORT_FIL_NAME,size_inode);	
			}
		}
	}
	return ERR_NONE;
}

int inode_findsector(const struct unix_filesystem *u, const struct inode *i, int32_t file_sec_off){
	M_REQUIRE_NON_NULL(u);
	M_REQUIRE_NON_NULL(i);
	
	int32_t size_file = inode_getsize(i);
	if(!(i->i_mode & IALLOC)){
		return ERR_UNALLOCATED_INODE;
	}
	if(file_sec_off < 0 || file_sec_off*SECTOR_SIZE >= size_file){
		return ERR_OFFSET_OUT_OF_RANGE;
	}

	if(size_file < ADDR_SMALL_LENGTH*SECTOR_SIZE){
		size_t index_sector = file_sec_off;
		return (i->i_addr)[index_sector];

	}else if(size_file < NB_INDIR_SECTORS*ADDRESSES_PER_SECTOR*SECTOR_SIZE){
		size_t index_sector = file_sec_off/ADDRESSES_PER_SECTOR;
		uint16_t data_sector = (i->i_addr)[index_sector];

		uint16_t data_addresses[ADDRESSES_PER_SECTOR] = {0};
		int read = sector_read(u->f, data_sector, data_addresses);

		if(read != ERR_NONE){
			return read;
		}
		uint16_t index_inode = file_sec_off%ADDRESSES_PER_SECTOR;
		
		return data_addresses[index_inode];
	}else{
		return ERR_FILE_TOO_LARGE;
	}
}


int inode_write(struct unix_filesystem *u, uint16_t inr, const struct inode *inode){
	M_REQUIRE_NON_NULL(u);
	M_REQUIRE_NON_NULL(inode);
	
	if(inr < ROOT_INUMBER || inr >= (u->s).s_isize*INODES_PER_SECTOR){ 
		return ERR_INODE_OUT_OF_RANGE; 
	}

	uint32_t num_sector = (u->s).s_inode_start + inr/INODES_PER_SECTOR; 
	uint16_t place_in_sector = inr%INODES_PER_SECTOR;
	struct inode_sector inodes_in_sector;
	int read = sector_read(u->f, num_sector, inodes_in_sector.inodes);
	if(read != ERR_NONE){
		return read;
	}
	
	memcpy(&(inodes_in_sector.inodes[place_in_sector]), inode, sizeof(struct inode));

	int write = sector_write(u->f, num_sector, inodes_in_sector.inodes);
	if(write != ERR_NONE){
		return write;
	} 

	return ERR_NONE;
}


int inode_alloc(struct unix_filesystem *u){
	M_REQUIRE_NON_NULL(u);

	int inr = bm_find_next(u->ibm);
	if(inr < ROOT_INUMBER){ 
		return inr;
	}

	bm_set(u->ibm, inr);

	return inr;
}


int inode_setsize(struct inode *inode, int new_size){
	M_REQUIRE_NON_NULL(inode);

	if(new_size < 0){
		return ERR_BAD_PARAMETER;
	}

	inode->i_size0 = (new_size >> 16);
	inode->i_size1 = (new_size & 0xFFFF);

	return ERR_NONE;
}
