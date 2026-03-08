#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>

#include "types.h"
#include "stat.h"
#include "fs.h"

// Global pointer to the memory-mapped file system image
void *img_ptr;
struct superblock *sb;

// Helper: Check if a command matches
int streq(const char *a, const char *b) {
    return strcmp(a, b) == 0;
}

// Get pointer to a specific block
void *get_block(uint bnum) {
    return img_ptr + (bnum * BSIZE);
}

// Get pointer to a specific inode
struct dinode *get_inode(uint inum) {
    uint block_num = IBLOCK(inum, (*sb));
    struct dinode *dip = (struct dinode *)get_block(block_num);
    int index = inum % IPB;
    return &dip[index];
}

void print_ls() {
    struct dinode *root_inode = get_inode(ROOTINO);
    
    // Iterate through direct blocks of the root directory
    for (int i = 0; i < NDIRECT; i++) {
        uint block_addr = root_inode->addrs[i];
        if (block_addr == 0) continue;

        struct dirent *de = (struct dirent *)get_block(block_addr);
        
        // Iterate through directory entries in this block
        for (int j = 0; j < BSIZE / sizeof(struct dirent); j++) {
            if (de[j].inum == 0) continue; // Empty entry

            struct dinode *file_inode = get_inode(de[j].inum);
            
            // Format: Name Type Inode Size
            // Note: Type is printed directly as integer per prompt example
            printf("%s %d %d %d\n", 
                de[j].name, 
                file_inode->type, 
                de[j].inum, 
                file_inode->size);
        }
    }
}

void do_cp(char *xv6_filename, char *linux_filename) {
    struct dinode *root_inode = get_inode(ROOTINO);
    struct dinode *target_inode = NULL;
    int found = 0;

    // 1. Find the file in the root directory
    for (int i = 0; i < NDIRECT; i++) {
        uint block_addr = root_inode->addrs[i];
        if (block_addr == 0) continue;

        struct dirent *de = (struct dirent *)get_block(block_addr);
        for (int j = 0; j < BSIZE / sizeof(struct dirent); j++) {
            if (de[j].inum != 0 && streq(de[j].name, xv6_filename)) {
                target_inode = get_inode(de[j].inum);
                found = 1;
                break;
            }
        }
        if (found) break;
    }

    if (!found) {
        // Specific error format required by the exercise
        fprintf(stderr, "File %s does not exist in the root directory\n", xv6_filename);
        exit(1);
    }

    // 2. Open/Create the Linux file for writing
    FILE *fp = fopen(linux_filename, "wb");
    if (!fp) {
        perror("Error creating linux file");
        exit(1);
    }

    // 3. Write data from xv6 inode to Linux file
    uint size = target_inode->size;
    uint bytes_written = 0;

    // Handle Direct Blocks
    for (int i = 0; i < NDIRECT && bytes_written < size; i++) {
        uint block_addr = target_inode->addrs[i];
        uint to_write = BSIZE;
        
        // Check if this is the last block (partial write)
        if (size - bytes_written < BSIZE) {
            to_write = size - bytes_written;
        }

        fwrite(get_block(block_addr), 1, to_write, fp);
        bytes_written += to_write;
    }

    // Handle Indirect Block (if file is larger than NDIRECT * BSIZE)
    if (bytes_written < size) {
        uint indirect_block_addr = target_inode->addrs[NDIRECT];
        uint *indirect_block = (uint *)get_block(indirect_block_addr);

        for (int i = 0; i < NINDIRECT && bytes_written < size; i++) {
            uint block_addr = indirect_block[i];
            uint to_write = BSIZE;

            if (size - bytes_written < BSIZE) {
                to_write = size - bytes_written;
            }

            fwrite(get_block(block_addr), 1, to_write, fp);
            bytes_written += to_write;
        }
    }

    fclose(fp);
}

int main(int argc, char *argv[]) {
    if (argc < 3) {
        fprintf(stderr, "Usage: hw5 <fs_image> <ls | cp xv6file linuxfile>\n");
        exit(1);
    }

    char *img_file = argv[1];
    char *command = argv[2];

    // Open the filesystem image
    int fd = open(img_file, O_RDONLY);
    if (fd < 0) {
        perror("Error opening image file");
        exit(1);
    }

    struct stat sbuf;
    if (fstat(fd, &sbuf) < 0) {
        perror("fstat error");
        exit(1);
    }

    // Map the file into memory for easy access
    img_ptr = mmap(NULL, sbuf.st_size, PROT_READ, MAP_PRIVATE, fd, 0);
    if (img_ptr == MAP_FAILED) {
        perror("mmap error");
        exit(1);
    }

    // The superblock is located at block 1 (Block 0 is bootblock)
    sb = (struct superblock *)(img_ptr + BSIZE);

    if (streq(command, "ls")) {
        print_ls();
    } else if (streq(command, "cp")) {
        if (argc != 5) {
            fprintf(stderr, "Usage: hw5 <fs_image> cp <xv6file> <linuxfile>\n");
            exit(1);
        }
        do_cp(argv[3], argv[4]);
    } else {
        fprintf(stderr, "Unknown command: %s\n", command);
        exit(1);
    }

    close(fd);
    return 0;
}