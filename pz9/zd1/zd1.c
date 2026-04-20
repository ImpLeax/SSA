#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

int main() {
    FILE *fp;
    char line[1024];
    uid_t my_uid = getuid();
    int found_others = 0;

    fp = popen("getent passwd", "r");
    if (fp == NULL) {
        printf("Error: Failed to run getent passwd\n");
        return 1;
    }

    printf("Current User UID: %d\n", my_uid);
    printf("Searching for other regular users (UID >= 1000)...\n\n");

    while (fgets(line, sizeof(line), fp) != NULL) {
        char *username = strtok(line, ":");
        strtok(NULL, ":"); 
        char *uid_str = strtok(NULL, ":");
        
        if (uid_str != NULL) {
            int uid = atoi(uid_str);
            if (uid >= 1000 && uid < 65534 && uid != my_uid) {
                printf("Found user: %s (UID: %d)\n", username, uid);
                found_others = 1;
            }
        }
    }

    if (!found_others) {
        printf("No other regular users found on this system.\n");
    }

    pclose(fp);
    return 0;
}
