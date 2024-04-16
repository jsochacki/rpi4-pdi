#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <stdint.h>

#define RANGES_PATH "/proc/device-tree/soc/ranges"
#define DEV_MEM "/dev/mem"
#define BUFFER_SIZE 16

#define BCM_ST_BASE    0x3000
#define BCM_GPIO_BASE  0x200000

void print_file_contents(const char* filename) {
    FILE *fp;
    ssize_t ret;
    unsigned char buffer[BUFFER_SIZE];

    fp = fopen(filename, "rb");
    if (!fp) {
        perror("Failed to open file");
        return;
    }

    ret = fread(buffer, 1, sizeof(buffer), fp);

    for (int i = 0; i < BUFFER_SIZE ; ++i) {
        printf("%02x ", buffer[i]);
        printf("\n");
    }

    printf("%02x\n", buffer[4] <<24);
    printf("%02x\n", buffer[5] <<16);
    printf("%02x\n", buffer[6] <<8);
    printf("%02x\n", buffer[7]);

    if (ret < 0) {
        perror("Failed to read file");
    }

    fclose(fp);
}

void print_mem_contents(const char* filename) {
    int fd = -1;

    fd = open(filename, O_RDWR | O_SYNC);
    if (fd < 0) {
        perror("Failed to open file");
        return;
    }

    off_t base = 0x7c000000;
    size_t size = 0x40000000;

    uint32_t *mem = mmap(0, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, base);
    if (mem == MAP_FAILED) return perror("Failed to map memory");

    // Save base addresses.  Divided by 4 for (uint32_t *) access.
    printf("%08x\n", BCM_GPIO_BASE);
    printf("%08x\n", BCM_ST_BASE);
    printf("%08x\n", base);
    printf("%08x\n", size);
    printf("%08x\n", mem);
    printf("%08x\n", mem + BCM_GPIO_BASE / 4);
    printf("%08x\n", mem + BCM_ST_BASE   / 4);
    printf("%08x\n", BCM_GPIO_BASE / 4);
    printf("%08x\n", BCM_ST_BASE / 4);

    close(fd);
}

int main() {
    printf("Contents of %s:\n", RANGES_PATH);
    print_file_contents(RANGES_PATH);

    printf("Contents of %s (first X bytes):\n", DEV_MEM);
    print_mem_contents(DEV_MEM);

    return 0;
}

