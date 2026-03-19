#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

int main() {
    printf("Process PID: %d\n", getpid());
    
    char *chunk1 = (char *)malloc(24);
    char *chunk2 = (char *)malloc(24);

    if (chunk1 == NULL || chunk2 == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        return 1;
    }

    printf("Chunk 1: %p\n", (void*)chunk1);
    printf("Chunk 2: %p\n", (void*)chunk2);

    printf("Executing buffer overflow...\n");
    memset(chunk1, 'A', 64);

    printf("Attempting to free chunk2...\n");
    free(chunk2);
    free(chunk1);

    return 0;
}
