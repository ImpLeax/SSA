#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main() {
    void *ptr = malloc(1024);
    
    if (ptr == NULL) {
        printf("Initial malloc failed.\n");
        return 1;
    }
    
    printf("Original memory allocated at: %p\n", ptr);

    void *temp = realloc(ptr, SIZE_MAX);

    if (temp == NULL) {
        printf("realloc failed to allocate SIZE_MAX. Returned NULL.\n");
        printf("Original pointer (ptr) is still valid: %p\n", ptr);
        
        free(ptr);
        printf("Original memory successfully freed.\n");
    } else {
        printf("realloc succeeded unexpectedly.\n");
        ptr = temp;
        free(ptr);
    }

    return 0;
}
