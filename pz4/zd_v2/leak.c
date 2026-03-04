#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

void print_proc_status() {
    FILE *file = fopen("/proc/self/status", "r");
    if (!file) return;
    
    char line[256];
    while (fgets(line, sizeof(line), file)) {
        if (strncmp(line, "VmSize:", 7) == 0 || strncmp(line, "VmRSS:", 6) == 0) {
            printf("  [OS] %s", line);
        }
    }
    fclose(file);
}

int main() {
    size_t total_allocated = 0;
    size_t chunk_size = 1024 * 1024;
    int iterations = 5;

    printf("Starting gradual memory leak simulation...\n");

    for (int i = 1; i <= iterations; i++) {
        void *ptr = malloc(chunk_size);
        
        if (ptr != NULL) {
            memset(ptr, 1, chunk_size);
            total_allocated += chunk_size;
            
            printf("\nIteration %d:\n", i);
            printf("  [Counter] Total tracked allocation: %zu bytes (%.1f MB)\n", 
                   total_allocated, total_allocated / (1024.0 * 1024.0));
            
            print_proc_status();
        }
        
        sleep(1);
    }

    printf("\nProgram finished without freeing memory.\n");
    return 0;
}
