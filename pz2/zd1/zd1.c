#include <stdio.h>
#include <time.h>
#include <limits.h>

int main() {

    printf("Size of time_t: %zu bytes (%zu bits)\n", sizeof(time_t), sizeof(time_t) * 8);

    if (sizeof(time_t) == 4) {
        printf("Architecture: 32-bit (vulnerable to Year 2038 problem)\n");

        time_t max_time = 0x7FFFFFFF; 
        printf("Maximum reachable time: %s\n", ctime(&max_time));
        
        max_time++; 
        printf("After overflow (+1 sec): %s\n", ctime(&max_time));
        printf("Note: The clock wrapped around to December 1901.\n");
    } 
    else if (sizeof(time_t) == 8) {
        printf("Architecture: 64-bit (safe from Year 2038 problem)\n");
        
        time_t max_time = 0x7FFFFFFFFFFFFFFF;
        printf("Current system time: %s\n", ctime(&max_time));
        
        printf("Theoretical limit is approx. 292 billion years from now.\n");
    }

    return 0;
}
