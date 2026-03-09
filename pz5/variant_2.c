#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main() {
    size_t num_elements = 0x4000000000000001; 
    size_t element_size = 4; 
    
    size_t total_size = num_elements * element_size;

    printf("Requested number of elements: %zu\n", num_elements);
    printf("Size of one element: %zu bytes\n", element_size);
    printf("Calculated size for malloc (after overflow): %zu bytes\n\n", total_size);

    int *buffer = (int *)malloc(total_size);
    if (buffer == NULL) {
        printf("Memory allocation failed!\n");
        return 1;
    }

    printf("Memory successfully allocated (but the block is too small)!\n");

    for (int i = 0; i < 4; i++) {
        buffer[i] = 0xDEADBEEF; 
    }

    printf("Data written. Heap metadata corrupted!\n");

    free(buffer);

    printf("Program successfully completed (this line is unlikely to execute).\n");

    return 0;
}
