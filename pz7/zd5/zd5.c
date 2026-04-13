#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>

void list_directory(const char *base_path) {
    char path[2048];
    struct dirent *entry;
    struct stat statbuf;
    DIR *dir = opendir(base_path);

    if (dir == NULL) {
        return;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
            continue;
        }

        snprintf(path, sizeof(path), "%s/%s", base_path, entry->d_name);
        printf("%s\n", path);

        if (stat(path, &statbuf) == 0) {
            if (S_ISDIR(statbuf.st_mode)) {
                list_directory(path);
            }
        }
    }

    closedir(dir);
}

int main(void) {
    list_directory(".");
    return EXIT_SUCCESS;
}
