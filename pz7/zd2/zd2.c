#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <string.h>

void print_permissions(mode_t mode) {
    printf(S_ISDIR(mode) ? "d" : "-");
    printf((mode & S_IRUSR) ? "r" : "-");
    printf((mode & S_IWUSR) ? "w" : "-");
    printf((mode & S_IXUSR) ? "x" : "-");
    printf((mode & S_IRGRP) ? "r" : "-");
    printf((mode & S_IWGRP) ? "w" : "-");
    printf((mode & S_IXGRP) ? "x" : "-");
    printf((mode & S_IROTH) ? "r" : "-");
    printf((mode & S_IWOTH) ? "w" : "-");
    printf((mode & S_IXOTH) ? "x" : "-");
    printf(" ");
}

int main(void) {
    DIR *dir;
    struct dirent *entry;
    struct stat file_stat;
    struct passwd *pwd;
    struct group *grp;
    char time_str[256];

    dir = opendir(".");
    if (dir == NULL) {
        perror("opendir");
        return EXIT_FAILURE;
    }

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            continue;
        }

        if (stat(entry->d_name, &file_stat) == -1) {
            continue;
        }

        print_permissions(file_stat.st_mode);

        printf("%lu ", (unsigned long)file_stat.st_nlink);

        pwd = getpwuid(file_stat.st_uid);
        if (pwd != NULL) {
            printf("%s ", pwd->pw_name);
        } else {
            printf("%d ", file_stat.st_uid);
        }

        grp = getgrgid(file_stat.st_gid);
        if (grp != NULL) {
            printf("%s ", grp->gr_name);
        } else {
            printf("%d ", file_stat.st_gid);
        }

        printf("%5ld ", (long)file_stat.st_size);

        struct tm *tm_info = localtime(&file_stat.st_mtime);
        strftime(time_str, sizeof(time_str), "%b %d %H:%M", tm_info);
        printf("%s ", time_str);

        printf("%s\n", entry->d_name);
    }

    closedir(dir);
    return EXIT_SUCCESS;
}
