#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main() {
    int32_t xa = 50000;
    int32_t xb = 50000;
    
    int32_t num = xa * xb;
    
    printf("xa = %d\n", xa);
    printf("xb = %d\n", xb);
    printf("num (signed int32_t) = %d\n", num);
    printf("num (cast to size_t) = %zu\n\n", (size_t)num);
    
    printf("Calling malloc...\n");
    void *ptr = malloc(num);
    
    if (ptr == NULL) {
        printf("malloc failed to allocate memory.\n");
    } else {
        printf("malloc succeeded.\n");
        free(ptr);
    }
    
    return 0;
}
