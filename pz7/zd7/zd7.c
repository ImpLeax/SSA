#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void) {
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;
    uid_t my_uid;
    char response[16];

    my_uid = getuid();
    dir = opendir(".");
    
    if (dir == NULL) {
        perror("opendir");
        return EXIT_FAILURE;
    }

    while ((entry = readdir(dir)) != NULL) {
        size_t len = strlen(entry->d_name);
        
        if (len > 2 && entry->d_name[len - 2] == '.' && entry->d_name[len - 1] == 'c') {
            if (stat(entry->d_name, &file_stat) == 0) {
                if (S_ISREG(file_stat.st_mode) && file_stat.st_uid == my_uid) {
                    printf("Found your C file: %s\n", entry->d_name);
                    printf("Grant read permission to others? (y/n): ");
                    
                    if (fgets(response, sizeof(response), stdin) != NULL) {
                        if (response[0] == 'y' || response[0] == 'Y') {
                            mode_t new_mode = file_stat.st_mode | S_IROTH;
                            if (chmod(entry->d_name, new_mode) == 0) {
                                printf("Read permission granted for %s\n\n", entry->d_name);
                            } else {
                                perror("chmod failed");
                            }
                        } else {
                            printf("Skipped %s\n\n", entry->d_name);
                        }
                    }
                }
            }
        }
    }

    closedir(dir);
    return EXIT_SUCCESS;
}
