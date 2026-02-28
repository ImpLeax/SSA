#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>

int main() {
    int server_fd;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int count = 0;

    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        perror("Socket creation failed");
        return 1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        return 1;
    }

    if (listen(server_fd, 1000) < 0) {
        perror("Listen failed");
        return 1;
    }

    printf("Server listening on port 8080...\n");

    while (1) {
        int new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen);
        if (new_socket < 0) {
            if (errno == EMFILE || errno == ENFILE) {
                printf("\n[Limit Exceeded] Cannot open more sockets!\n");
                printf("Total active connections: %d\n", count);
                break;
            }
            perror("Accept error");
            break;
        }
        count++;
        printf("Connection #%d accepted (FD: %d)\n", count, new_socket);
    }

    close(server_fd);
    return 0;
}
