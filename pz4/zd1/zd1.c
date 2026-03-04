#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main() {
    printf("--- Memory Limits Analysis ---\n\n");

    printf("Size of size_t: %zu bytes\n", sizeof(size_t));
    printf("Max of size_t (SIZE_MAX): %zu bytes\n", SIZE_MAX);
    printf("Max of ptrdiff_t (PTRDIFF_MAX): %td bytes\n\n", PTRDIFF_MAX);

    printf("Attempting to allocate %td bytes (PTRDIFF_MAX)...\n", PTRDIFF_MAX);
    void *ptr_huge = malloc(PTRDIFF_MAX);
    
    if (ptr_huge == NULL) {
        printf("Failed to allocate PTRDIFF_MAX memory.\n\n");
    } else {
        printf("Unexpectedly succeeded in allocating PTRDIFF_MAX!\n\n");
        free(ptr_huge);
    }

    size_t one_tb = 1024ULL * 1024 * 1024 * 1024;
    printf("Attempting to allocate %zu bytes (1 TB)...\n", one_tb);
    void *ptr_large = malloc(one_tb);
    
    if (ptr_large == NULL) {
        printf("Failed to allocate 1 TB.\n");
    } else {
        printf("Succeeded in allocating 1 TB of virtual memory!\n");
        free(ptr_large);
    }

    printf("\nProgram finished.\n");
    return 0;
}
