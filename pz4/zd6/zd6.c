#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("--- Test 1: realloc(NULL, size) ---\n");
    
    void *ptr1 = realloc(NULL, 1024);
    
    if (ptr1 != NULL) {
        printf("realloc(NULL, 1024) acted like malloc. Allocated at: %p\n", ptr1);
    } else {
        printf("realloc(NULL, 1024) failed.\n");
        return 1;
    }

    printf("\n--- Test 2: realloc(ptr, 0) ---\n");
    
    void *ptr2 = realloc(ptr1, 0);
    
    if (ptr2 == NULL) {
        printf("realloc(ptr, 0) returned NULL. The memory was freed.\n");
    } else {
        printf("realloc(ptr, 0) returned non-NULL: %p\n", ptr2);
        free(ptr2);
    }

    return 0;
}
