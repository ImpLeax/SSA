#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main() {
    int fd;
    unsigned char initial_data[] = {4, 5, 2, 2, 3, 3, 7, 9, 1, 5};
    unsigned char buffer[4];
    int i;

    fd = open("data.bin", O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        printf("Error opening file\n");
        return 1;
    }

    write(fd, initial_data, sizeof(initial_data));

    lseek(fd, 3, SEEK_SET);

    read(fd, buffer, 4);

    printf("Buffer contents:\n");
    for (i = 0; i < 4; i++) {
        printf("%d ", buffer[i]);
    }
    printf("\n");

    close(fd);
    return 0;
}
