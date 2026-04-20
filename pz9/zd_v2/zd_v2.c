#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>

int main(int argc, char *argv[]) {
    if (argc != 4) {
        printf("Usage: %s <filename> <start_hour> <end_hour>\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    int start_hour = atoi(argv[2]);
    int end_hour = atoi(argv[3]);

    FILE *f = fopen(filename, "a");
    if (f) {
        fputs("Protected content.\n", f);
        fclose(f);
    }

    printf("Monitoring file: %s\n", filename);
    printf("Allowed window: %02d:00 - %02d:00\n", start_hour, end_hour);
    printf("Press Ctrl+C to stop the monitor.\n\n");

    while (1) {
        time_t t = time(NULL);
        struct tm *tm_info = localtime(&t);
        int current_hour = tm_info->tm_hour;

        if (current_hour >= start_hour && current_hour < end_hour) {
            chmod(filename, 0600);
            printf("[%02d:%02d:%02d] STATUS: UNLOCKED (Permissions: 600)\n", 
                   tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
        } else {
            chmod(filename, 0000);
            printf("[%02d:%02d:%02d] STATUS: LOCKED (Permissions: 000)\n", 
                   tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
        }

        sleep(10);
    }

    return 0;
}
