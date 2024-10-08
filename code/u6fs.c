/**
 * @file u6fs.c
 * @brief Command line interface
 *
 * @author Édouard Bugnion, Ludovic Mermod
 * @date 2022
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "error.h"
#include "mount.h"
#include "u6fs_utils.h"
#include "inode.h"
#include "direntv6.h"
#include "bmblock.h"


#define FTELL_ERROR -1L

//helper function for add method in shell
int utils_add_file(struct unix_filesystem *u, const char* destination_file, const char* source_file);
    
/* *************************************************** *
 * TODO WEEK 04-07: Add more messages                  *
 * *************************************************** */
static void usage(const char *execname, int err)
{
    if (err == ERR_INVALID_COMMAND) {
        pps_printf("Available commands:\n");
        pps_printf("%s <disk> sb\n", execname);
        pps_printf("%s <disk> inode\n", execname);
        pps_printf("%s <disk> cat1 <inr>\n", execname);
        pps_printf("%s <disk> shafiles\n", execname);
        pps_printf("%s <disk> tree\n", execname);
        pps_printf("%s <disk> fuse <mountpoint>\n", execname);
        pps_printf("%s <disk> bm", execname);
        pps_printf("%s <disk> mkdir </path/to/newdir>", execname); //WEEK11
        pps_printf("%s <disk> add <dest> <disk>", execname);  //pas sur de la commande, je l'ai un peu inventé mdrr
    } else if (err > ERR_FIRST && err < ERR_LAST) {
        pps_printf("%s: Error: %s\n", execname, ERR_MESSAGES[err - ERR_FIRST]);
    } else {
        pps_printf("%s: Error: %d (UNDEFINED ERROR)\n", execname, err);
    }
}



#define CMD(a, b) (strcmp(argv[2], a) == 0 && argc == (b))

/* *************************************************** *
 * TODO WEEK 04-11: Add more commands                  *
 * *************************************************** */
/**
 * @brief Runs the command requested by the user in the command line, or returns ERR_INVALID_COMMAND if the command is not found.
 *
 * @param argc (int) the number of arguments in the command line
 * @param argv (char*[]) the arguments of the command line, as passed to main()
 */
int u6fs_do_one_cmd(int argc, char *argv[])
{
    if (argc < 3) return ERR_INVALID_COMMAND;

    struct unix_filesystem u = {0};
    int error = mountv6(argv[1], &u), err2 = 0;

    if (error != ERR_NONE) {
        debug_printf("Could not mount fs%s", "\n");
        return error;
    }

    if (CMD("sb", 3)) {
        error = utils_print_superblock(&u);
    }else if (CMD("inode", 3)){
        error = inode_scan_print(&u);
    }else if (CMD("cat1", 4)){
        uint16_t inr = (uint16_t)atoi(argv[3]);
        error = utils_cat_first_sector(&u, inr);
    }else if (CMD("shafiles", 3)){
        error = utils_print_sha_allfiles(&u);
    }else if (CMD("tree", 3)){
        error = direntv6_print_tree(&u, ROOT_INUMBER, "");
    }else if (CMD("fuse", 4)){
        error = u6fs_fuse_main(&u, argv[3]);
    }else if(CMD("bm", 3)){
        error = utils_print_bitmaps(&u);
    }else if(CMD("mkdir", 4)){
        int add = direntv6_create(&u, argv[3], IWRITE | IREAD | IEXEC | IFDIR);
        error = (add < ERR_NONE) ? add : ERR_NONE;
    }else if(CMD("add", 5)){
        error = utils_add_file(&u, argv[3], argv[4]);
    }else{
        error = ERR_INVALID_COMMAND;
    }
    err2 = umountv6(&u);
    return (error == ERR_NONE ? err2 : error);
}

#ifndef FUZZ
/**
 * @brief main function, runs the requested command and prints the resulting error if any.
 */
int main(int argc, char *argv[])
{
    int ret = u6fs_do_one_cmd(argc, argv);
    if (ret != ERR_NONE) {
        usage(argv[0], ret);
    }
    return ret;
}
#endif



int utils_add_file(struct unix_filesystem *u, const char* destination_file, const char* source_file){
    M_REQUIRE_NON_NULL(u);
    M_REQUIRE_NON_NULL(destination_file);
    M_REQUIRE_NON_NULL(source_file);

    FILE* f = fopen(source_file, "rb");
    if(f == NULL){
        return ERR_IO;
    }
    int seek1 = fseek(f, 0L, SEEK_END);
    if(seek1){
        return ERR_IO;
    }
    long int size = ftell(f);
    if(size == FTELL_ERROR){
        return ERR_IO;
    }
    int seek2 = fseek(f, 0L, SEEK_SET);
    if(seek2){
        return ERR_IO;
    }

    uint8_t buf[ADDR_SMALL_LENGTH*SECTOR_SIZE] = {0};
    unsigned long nb_read = fread(buf, 1, size, f);
    if(nb_read != size){
        return ERR_IO;
    }

    return direntv6_addfile(u, destination_file, IWRITE | IREAD | IEXEC, buf, (size_t) size);
}
