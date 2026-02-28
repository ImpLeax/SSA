#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <errno.h>

void handle_sigxfsz(int signum) {
    printf("\n[SIGXFSZ Signal] File size limit exceeded!\n");
    exit(1);
}

int main(int argc, char *argv[]) {
    if (argc != 3) {
        printf("Program need two arguments\n");
        return 1;
    }

    if (signal(SIGXFSZ, handle_sigxfsz) == SIG_ERR) {
        return 1;
    }

    int fd_in = open(argv[1], O_RDONLY);
    if (fd_in < 0) {
        printf("Cannot open file %s for reading\n", argv[1]);
        return 1;
    }

    int fd_out = open(argv[2], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd_out < 0) {
        printf("Cannot open file %s for writing\n", argv[2]);
        close(fd_in);
        return 1;
    }

    char buffer[4096];
    ssize_t bytes_read, bytes_written;

    while ((bytes_read = read(fd_in, buffer, sizeof(buffer))) > 0) {
        bytes_written = write(fd_out, buffer, bytes_read);
        if (bytes_written == -1 && errno == EFBIG) {
            printf("\nWrite error: File size limit exceeded!\n");
            break;
        }
    }

    close(fd_in);
    close(fd_out);

    return 0;
}
