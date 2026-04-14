#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <signal.h>

int main() {
    struct rlimit rl;
    int fd;
    char buffer[] = "This string contains more than ten bytes.";
    ssize_t bytes_written;

    rl.rlim_cur = 10;
    rl.rlim_max = 10;
    setrlimit(RLIMIT_FSIZE, &rl);

    signal(SIGXFSZ, SIG_IGN);

    fd = open("test_output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        printf("Error opening file\n");
        return 1;
    }

    bytes_written = write(fd, buffer, 41);

    printf("Requested bytes to write: 41\n");
    printf("Actual bytes written: %zd\n", bytes_written);

    close(fd);
    return 0;
}
