#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#define LINES_PER_PAGE 20
#define MAX_LINE_LENGTH 2048

int wait_for_keypress(void) {
    struct termios old_term, new_term;
    int ch;
    FILE *tty;

    tty = fopen("/dev/tty", "r+");
    if (tty == NULL) {
        return -1;
    }

    tcgetattr(fileno(tty), &old_term);
    new_term = old_term;
    new_term.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(fileno(tty), TCSANOW, &new_term);

    ch = fgetc(tty);

    tcsetattr(fileno(tty), TCSANOW, &old_term);
    fclose(tty);

    return ch;
}

void process_file(const char *filename) {
    FILE *file;
    char line[MAX_LINE_LENGTH];
    int line_count = 0;
    int key;

    file = fopen(filename, "r");
    if (file == NULL) {
        perror(filename);
        return;
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        printf("%s", line);
        line_count++;

        if (line_count == LINES_PER_PAGE) {
            key = wait_for_keypress();
            if (key == 'q' || key == 'Q') {
                fclose(file);
                exit(EXIT_SUCCESS);
            }
            line_count = 0;
        }
    }

    fclose(file);
}

int main(int argc, char *argv[]) {
    int i;

    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file1> [file2 ...]\n", argv[0]);
        return EXIT_FAILURE;
    }

    for (i = 1; i < argc; i++) {
        process_file(argv[i]);
    }

    return EXIT_SUCCESS;
}
