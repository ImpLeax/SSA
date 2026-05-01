#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();

    if (pid < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    } 
    else if (pid == 0) {
        printf("=== Child Process ===\n");
        printf("My PID (Child): %d\n", getpid());
        printf("My Parent's PID: %d\n", getppid());
        exit(EXIT_SUCCESS); 
    } 
    else {
        wait(NULL); 
        printf("=== Parent Process ===\n");
        printf("Child process finished successfully. Parent (PID: %d) is also exiting.\n", getpid());
    }

    return 0;
}
