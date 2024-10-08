#include <inttypes.h>
#include <stdlib.h>
#include <string.h>
#include "error.h"
#include "filev6.h"
#include "direntv6.h"
#include "unixv6fs.h"
#include "inode.h"

#define SUCCESS 1

// Prototype
int direntv6_dirlookup_core(const struct unix_filesystem *u, uint16_t inr, const char *entry, size_t len);


int direntv6_opendir(const struct unix_filesystem *u, uint16_t inr, struct directory_reader *d){
    M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(d);

    struct filev6 fv6 = {0};

    int read = filev6_open(u, inr, &fv6);
    if(read != ERR_NONE)
    {
        return read;
    }
    if(!(fv6.i_node.i_mode & IFDIR)){
        return ERR_INVALID_DIRECTORY_INODE;
    }
    
    d->fv6 = fv6;
    d->cur = 0;
    d->last = 0;
    memset(d->dirs, 0, sizeof(d->dirs));
    return ERR_NONE;
}


int direntv6_readdir(struct directory_reader *d, char *name, uint16_t *child_inr){
    M_REQUIRE_NON_NULL(d);
    M_REQUIRE_NON_NULL(name);
    M_REQUIRE_NON_NULL(child_inr);
    if(d->cur == d->last){
        d->cur = 0;
        int bytes_read = filev6_readblock(&(d->fv6), d->dirs);
        if(bytes_read < 0){
            return bytes_read;
        }
        d->last = bytes_read/sizeof(struct direntv6); 
        if(d->last == 0){
            return ERR_NONE;
        }
    }
    strncpy(name, (d->dirs)[d->cur].d_name, DIRENT_MAXLEN);

    *child_inr = (d->dirs)[d->cur].d_inumber;

    if(d->cur < d->last){
        d->cur += 1;
    }
    return SUCCESS;
}


int direntv6_print_tree(const struct unix_filesystem *u, uint16_t inr, const char *prefix){
    M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(prefix);

    struct inode i;

    int read = inode_read(u, inr, &i);
    if(read != ERR_NONE){
        return read;
    }

    if(!(i.i_mode & IFDIR)){
        pps_printf("%s %s\n", SHORT_FIL_NAME, prefix);
        return ERR_NONE;
    }
    pps_printf("%s %s/\n", SHORT_DIR_NAME, prefix);

    struct directory_reader dir_read;;
    uint16_t child_inr;
    char name[DIRENT_MAXLEN+1];

    int open = direntv6_opendir(u, inr, &dir_read);
    if(open != ERR_NONE){
        return open;
    }
    do{
        read = direntv6_readdir(&dir_read, name, &child_inr);
        if(read != SUCCESS){
            return read;
        }
        size_t new_len = (strlen(prefix)+strlen(name)+2);
        char* new_prefix = calloc(new_len, sizeof(char));
        snprintf(new_prefix, new_len, "%s/%s", prefix, name);
        int recursive_print = direntv6_print_tree(u, child_inr, new_prefix);
        free(new_prefix);
        new_prefix = NULL;

        if(recursive_print != ERR_NONE){
            return recursive_print;
        }
    }while(read == SUCCESS);

    return ERR_NONE;
}


int direntv6_dirlookup(const struct unix_filesystem *u, uint16_t inr, const char* entry){
    M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(entry);

    return direntv6_dirlookup_core(u, inr, entry, (size_t)strlen(entry));
}


int direntv6_dirlookup_core(const struct unix_filesystem *u, uint16_t inr, const char* entry, size_t len){
    if (len == 0){
        return inr;
    }

    size_t pos = 0;
    size_t new_len = len;
    char repertory_name[DIRENT_MAXLEN+1] = {0};
    while(entry[pos] == '/'){
        entry += 1;
        new_len -= 1;
    }
    if(strlen(entry) == 0){
        return inr;
    }
    while(entry[pos] != '/' && entry[pos] != '\0'){
        repertory_name[pos] = entry[pos];
        pos += 1;
        new_len -= 1;
    }
    entry += pos; //shift the pointer to the start of the next '/'

    struct directory_reader d = {0};

    int read = direntv6_opendir(u, inr, &d);
    if(read != ERR_NONE){
        return read;
    }

    uint16_t newInr;
    char name[DIRENT_MAXLEN+1];
    
    do{
        read = direntv6_readdir(&d, name, &newInr);
        if (read < 0){
            return read;
        }
    } while (read > 0 && strncmp(repertory_name, name, DIRENT_MAXLEN));
    

    if(read == 0){
        return ERR_NO_SUCH_FILE;
    }

    int inode = direntv6_dirlookup_core(u, newInr, entry, new_len);

    return inode;
}


int direntv6_create(struct unix_filesystem *u, const char *entry, uint16_t mode){
    M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(entry);

    size_t len = strlen(entry);
    
    const char* end_entry = entry + len - 1;
    while(end_entry[0] == '/' && end_entry >= entry){
        end_entry -= 1;
        len -= 1;
    }

    char* temp_entry = calloc(len+1, 1);
    if(temp_entry == NULL){
        return ERR_NOMEM;
    }
    strncpy(temp_entry, entry, len);

    char* start_relativ = strrchr(temp_entry, '/'); //start from the end
    size_t len_parent = 0;
    if(start_relativ == NULL){
        start_relativ = temp_entry; 
    }else{
        len_parent = start_relativ - temp_entry;
        start_relativ += 1; 
    }
    
    if(strlen(start_relativ) > DIRENT_MAXLEN){
        free(temp_entry);
        temp_entry = NULL;
        return ERR_FILENAME_TOO_LONG;
    } 

    char* parent_path = calloc(len_parent+1, 1);
    if(parent_path == NULL){
        free(temp_entry);
        temp_entry = NULL;
        return ERR_NOMEM;
    }
    strncpy(parent_path, temp_entry, len_parent);

    int parent_inr = direntv6_dirlookup(u, ROOT_INUMBER, parent_path);
    if(parent_inr < ROOT_INUMBER){
        free(temp_entry);
        temp_entry = NULL;
        free(parent_path);
        parent_path = NULL;
        return ERR_NO_SUCH_FILE;
    }
    free(parent_path);
    parent_path = NULL;

    int inr_if_exists = direntv6_dirlookup(u, ROOT_INUMBER, entry);
    if(inr_if_exists >= ROOT_INUMBER){
        free(temp_entry);
        temp_entry = NULL;
        return ERR_FILENAME_ALREADY_EXISTS; 
    }

    struct filev6 child_fv6 = {0};
    int create_file = filev6_create(u, mode, &child_fv6);
    if(create_file != ERR_NONE){
        free(temp_entry);
        temp_entry = NULL;
        return create_file;
    }
    

    struct direntv6 direntv6 = {0}; 
    direntv6.d_inumber = child_fv6.i_number;
    strncpy(direntv6.d_name, start_relativ, len - len_parent);
    
    free(temp_entry); //no need to free start_relative, place in memory already freed
    temp_entry = NULL;

    struct inode parent_inode = {0};
    int read_inode = inode_read(u, parent_inr, &parent_inode);
    struct filev6 fv6 = {u, parent_inr, parent_inode, 0};

    int write = filev6_writebytes(&fv6, &direntv6, sizeof(struct direntv6));
    if(write != ERR_NONE){
        return write;
    }

    return child_fv6.i_number;
}


int direntv6_addfile(struct unix_filesystem *u, const char *entry, uint16_t mode, char *buf, size_t size){
    M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(entry);
    M_REQUIRE_NON_NULL(buf);

    int inr_direntv6 = direntv6_create(u, entry, mode);
    if(inr_direntv6 < 0){
        return inr_direntv6;
    }

    struct filev6 fv6 = {0};
    int open = filev6_open(u, inr_direntv6, &fv6);  
    if(open != ERR_NONE){
        return open;
    }

    int write = filev6_writebytes(&fv6, buf, size);
    if(write != ERR_NONE){
        return write;
    }

    return ERR_NONE;
}
