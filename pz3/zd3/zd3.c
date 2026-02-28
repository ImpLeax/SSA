#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <signal.h>
#include <unistd.h>

void sigxfsz_handler(int sig) {
    printf("\nFile size limit exceeded! Program stopped.\n");
    exit(1);
}

int imitate_throw() {
    return 1 + rand() % 6;
}

int main() {
    srand(time(NULL));

    signal(SIGXFSZ, sigxfsz_handler);

    FILE *file = fopen("throw_results.txt", "w");
    if (file == NULL) {
        printf("Cannot open file!\n");
        return 1;
    }

    int i = 0;
    while (i < 10000) {
        fprintf(file, "Throw #%d: %d\n", i, imitate_throw());
        i++;
    }

    fclose(file);
    return 0;
}
