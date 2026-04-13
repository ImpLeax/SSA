#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

void target_code(void) {
    volatile long long sum = 0;
    for (long long i = 0; i < 500000000LL; i++) {
        sum += i;
    }
}

int main(void) {
    struct timeval start, end;
    long long start_ms, end_ms, elapsed_ms;

    gettimeofday(&start, NULL);

    target_code();

    gettimeofday(&end, NULL);

    start_ms = (start.tv_sec * 1000LL) + (start.tv_usec / 1000LL);
    end_ms = (end.tv_sec * 1000LL) + (end.tv_usec / 1000LL);
    elapsed_ms = end_ms - start_ms;

    printf("Execution time: %lld milliseconds\n", elapsed_ms);

    return EXIT_SUCCESS;
}
