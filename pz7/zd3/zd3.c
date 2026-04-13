#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 2048

int main(int argc, char *argv[]) {
    FILE *file;
    char line[MAX_LINE_LENGTH];

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <word> <file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    file = fopen(argv[2], "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        if (strstr(line, argv[1]) != NULL) {
            printf("%s", line);
        }
    }

    fclose(file);
    return EXIT_SUCCESS;
}
