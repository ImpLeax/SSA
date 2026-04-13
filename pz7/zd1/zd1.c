#include <stdio.h>
#include <stdlib.h>

int main(void) {
    FILE *rwho_pipe;
    FILE *more_pipe;
    char buffer[1024];

    rwho_pipe = popen("rwho", "r");
    if (rwho_pipe == NULL) {
        perror("Error opening rwho pipe");
        exit(EXIT_FAILURE);
    }

    more_pipe = popen("more", "w");
    if (more_pipe == NULL) {
        perror("Error opening more pipe");
        pclose(rwho_pipe);
        exit(EXIT_FAILURE);
    }

    while (fgets(buffer, sizeof(buffer), rwho_pipe) != NULL) {
        fputs(buffer, more_pipe);
    }

    pclose(rwho_pipe);
    pclose(more_pipe);

    return 0;
}
