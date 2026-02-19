#include <stdio.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <unistd.h>

int main() {
    printf("PID: %d\n\n", getpid());

    void *ptr_malloc_small = malloc(1024); 
    printf("1. malloc (1 KB) address:   %p\n", ptr_malloc_small);

    void *ptr_malloc_large = malloc(1024 * 1024 * 10); 
    printf("2. malloc (10 MB) address:  %p\n", ptr_malloc_large);

    void *ptr_mmap = mmap(NULL, 1024 * 1024 * 10, PROT_READ | PROT_WRITE, 
                          MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (ptr_mmap == MAP_FAILED) {
        perror("mmap failed");
        return 1;
    }
    printf("3. mmap (10 MB) address:    %p\n", ptr_mmap);

    free(ptr_malloc_small);
    free(ptr_malloc_large);
    munmap(ptr_mmap, 1024 * 1024 * 10); 

    return 0;
}
