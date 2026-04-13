#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

int is_dir(const struct dirent *entry) {
    struct stat statbuf;
    
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) {
        return 0;
    }
    
    if (stat(entry->d_name, &statbuf) == 0) {
        if (S_ISDIR(statbuf.st_mode)) {
            return 1;
        }
    }
    
    return 0;
}

int main(void) {
    struct dirent **namelist;
    int n;
    int i;

    n = scandir(".", &namelist, is_dir, alphasort);
    
    if (n < 0) {
        perror("scandir");
        return EXIT_FAILURE;
    }

    for (i = 0; i < n; i++) {
        printf("%s\n", namelist[i]->d_name);
        free(namelist[i]);
    }
    
    free(namelist);

    return EXIT_SUCCESS;
}
