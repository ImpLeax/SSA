#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[]) {
    int i;
    double n;

    srand((unsigned int)time(NULL));

    printf("Sequence (0.0 to 1.0):\n");
    for (i = 0; i < 5; i++) {
        printf("%f\n", (double)rand() / RAND_MAX);
    }

    if (argc > 1) {
        n = atof(argv[1]);
    } else {
        n = 100.0;
    }

    printf("\nSequence (0.0 to %f):\n", n);
    for (i = 0; i < 5; i++) {
        printf("%f\n", ((double)rand() / RAND_MAX) * n);
    }

    return EXIT_SUCCESS;
}
