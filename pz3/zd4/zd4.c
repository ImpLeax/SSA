#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <time.h>

void handle_sigxcpu(int signum) {
    printf("\n\n[SIGXCPU Signal] CPU time limit exceeded!\n");
    printf("Program terminated gracefully. Thanks for playing.\n");
    exit(0);
}

void generate_lottery(int k, int n) {
    int numbers[n];
    for (int i = 0; i < n; i++) {
        numbers[i] = i + 1;
    }

    for (int i = 0; i < k; i++) {
        int j = i + rand() % (n - i);
        int temp = numbers[i];
        numbers[i] = numbers[j];
        numbers[j] = temp;
    }

    for (int i = 0; i < k; i++) {
        printf("%d ", numbers[i]);
    }
    printf("| ");
}

int main() {
    if (signal(SIGXCPU, handle_sigxcpu) == SIG_ERR) {
        perror("Error setting signal handler");
        return 1;
    }

    srand(time(NULL));

    printf("Starting lottery generation...\n");
    
    while (1) {
        generate_lottery(7, 49);
        generate_lottery(6, 36);
        printf("\n");
    }

    return 0;
}
