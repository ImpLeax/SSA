#define _GNU_SOURCE
#include <stdlib.h>

struct sbar {
    int data;
};

int main() {
    struct sbar *ptr, *newptr;
    
    ptr = calloc(1000, sizeof(struct sbar));
    if (!ptr) return 1;

    newptr = reallocarray(ptr, 500, sizeof(struct sbar));
    
    if (newptr) {
        free(newptr);
    } else {
        free(ptr);
    }

    return 0;
}
