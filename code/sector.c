#include "error.h"
#include "unixv6fs.h"


int sector_read(FILE *f, uint32_t sector, void *data){
	M_REQUIRE_NON_NULL(f);
	M_REQUIRE_NON_NULL(data);

	int success = fseek(f, sector*SECTOR_SIZE, SEEK_SET);
	if(success){
		return ERR_IO;
	}
	unsigned long nb_read = fread(data, SECTOR_SIZE, 1, f);
	if(nb_read != 1){
		return ERR_IO;
	}
	
	return ERR_NONE;
}


int sector_write(FILE *f, uint32_t sector, const void *data){
	M_REQUIRE_NON_NULL(f);
	M_REQUIRE_NON_NULL(data);

	int success = fseek(f, sector*SECTOR_SIZE, SEEK_SET);
	if(success){
		return ERR_IO;
	}
	size_t nb_written = fwrite(data, SECTOR_SIZE, 1, f);
	if(nb_written != 1){  
		return ERR_IO;
	}

	return ERR_NONE;
}

