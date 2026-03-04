#include <stdio.h>
#include <stdlib.h>

int main() {
    void *ptr = NULL;
    int condition = 2;

    printf("Starting flawed loop...\n");

    while (condition > 0) {
        if (!ptr) {
            ptr = malloc(1024);
            printf("Allocated memory at: %p\n", ptr);
        }

        printf("Using memory at: %p\n", ptr);

        free(ptr);
        printf("Freed memory at: %p\n", ptr);

        condition--;
    }

    printf("Program finished successfully.\n");
    return 0;
}
