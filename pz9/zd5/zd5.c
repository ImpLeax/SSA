#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void check_access(const char *filename) {
    printf("Read access: ");
    if (access(filename, R_OK) == 0) {
        printf("YES\t");
    } else {
        printf("NO\t");
    }
    
    printf("Write access: ");
    if (access(filename, W_OK) == 0) {
        printf("YES\n");
    } else {
        printf("NO\n");
    }
}

int main() {
    const char *filename = "test_temp_file.txt";
    
    FILE *fp = fopen(filename, "w");
    if (fp != NULL) {
        fputs("Test data\n", fp);
        fclose(fp);
    }
    
    printf("Changing file owner to root...\n");
    system("sudo chown root test_temp_file.txt");
    
    printf("\nSetting permissions to 600 (root: rw, others: none)...\n");
    system("sudo chmod 600 test_temp_file.txt");
    check_access(filename);
    
    printf("\nSetting permissions to 644 (root: rw, others: read-only)...\n");
    system("sudo chmod 644 test_temp_file.txt");
    check_access(filename);
    
    printf("\nSetting permissions to 666 (root: rw, others: read-write)...\n");
    system("sudo chmod 666 test_temp_file.txt");
    check_access(filename);
    
    system("sudo rm -f test_temp_file.txt");
    
    return 0;
}
