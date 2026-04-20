#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void test_file_access(const char *path) {
    char cmd[256];
    FILE *f;

    printf("--- Inspecting: %s ---\n", path);
    snprintf(cmd, sizeof(cmd), "ls -ld %s", path);
    system(cmd);

    printf("Attempting READ: ");
    f = fopen(path, "r");
    if (f) {
        printf("SUCCESS\n");
        fclose(f);
    } else {
        printf("DENIED\n");
    }

    printf("Attempting WRITE: ");
    f = fopen(path, "a");
    if (f) {
        printf("SUCCESS\n");
        fclose(f);
    } else {
        printf("DENIED\n");
    }

    printf("Attempting EXECUTE: ");
    if (access(path, X_OK) == 0) {
        printf("SUCCESS\n");
    } else {
        printf("DENIED\n");
    }

    printf("Attempting BYPASS (change permissions): ");
    snprintf(cmd, sizeof(cmd), "chmod 777 %s 2>/dev/null", path);
    if (system(cmd) == 0) {
        printf("SUCCESS\n");
    } else {
        printf("DENIED\n");
    }
    printf("\n");
}

int main() {
    char home_path[256];
    snprintf(home_path, sizeof(home_path), "%s/my_test_file.txt", getenv("HOME"));
    
    char create_cmd[256];
    snprintf(create_cmd, sizeof(create_cmd), "touch %s", home_path);
    system(create_cmd);

    test_file_access(home_path);
    test_file_access("/etc/passwd");
    test_file_access("/etc/shadow");
    test_file_access("/usr/bin/whoami");

    char rm_cmd[256];
    snprintf(rm_cmd, sizeof(rm_cmd), "rm -f %s", home_path);
    system(rm_cmd);

    return 0;
}
