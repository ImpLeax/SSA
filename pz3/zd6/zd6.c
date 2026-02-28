#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

void handle_sigsegv(int signum) {
    printf("\n[SIGSEGV Signal] Stack segment size limit exceeded!\n");
    exit(1);
}

void recursive_function(int depth) {
    char buffer[1024];
    
    if (depth % 1000 == 0) {
        printf("Current recursion depth: %d\n", depth);
    }
    
    recursive_function(depth + 1);
}

int main() {
    stack_t alt_stack;
    alt_stack.ss_sp = malloc(SIGSTKSZ);
    if (alt_stack.ss_sp == NULL) {
        printf("Memory allocation failed\n");
        return 1;
    }
    alt_stack.ss_size = SIGSTKSZ;
    alt_stack.ss_flags = 0;
    
    if (sigaltstack(&alt_stack, NULL) == -1) {
        printf("sigaltstack failed\n");
        return 1;
    }

    struct sigaction sa;
    sa.sa_handler = handle_sigsegv;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_ONSTACK;
    
    if (sigaction(SIGSEGV, &sa, NULL) == -1) {
        printf("sigaction failed\n");
        return 1;
    }

    printf("Starting infinite recursion to demonstrate stack overflow...\n");
    recursive_function(1);

    return 0;
}
