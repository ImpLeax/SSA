#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <ctype.h>

char valid_shells[64][256];
int num_shells = 0;

void load_shells(void) {
    FILE *f = fopen("/etc/shells", "r");
    if (!f) return;
    char line[256];
    while (fgets(line, sizeof(line), f)) {
        if (line[0] == '#' || line[0] == '\n') continue;
        line[strcspn(line, "\n")] = 0;
        strncpy(valid_shells[num_shells], line, 255);
        num_shells++;
    }
    fclose(f);
}

int is_valid_shell(const char *path) {
    for (int i = 0; i < num_shells; i++) {
        if (strcmp(path, valid_shells[i]) == 0) return 1;
    }
    return 0;
}

void check_process(const char *pid_str) {
    char path[512], stat_buf[1024];
    snprintf(path, sizeof(path), "/proc/%s/stat", pid_str);
    
    FILE *f = fopen(path, "r");
    if (!f) return;
    if (!fgets(stat_buf, sizeof(stat_buf), f)) {
        fclose(f);
        return;
    }
    fclose(f);

    char *start = strchr(stat_buf, '(');
    char *end = strrchr(stat_buf, ')');
    if (!start || !end) return;
    
    *start = '\0';
    *end = '\0';
    char *comm = start + 1;
    
    int ppid, pgrp, sid, tty_nr;
    if (sscanf(end + 2, "%*c %d %d %d %d", &ppid, &pgrp, &sid, &tty_nr) != 4) return;
    
    if (tty_nr == 0 || ppid == 0) return;

    char ppid_stat_path[512], ppid_stat_buf[1024];
    snprintf(ppid_stat_path, sizeof(ppid_stat_path), "/proc/%d/stat", ppid);
    f = fopen(ppid_stat_path, "r");
    if (!f) return;
    if (!fgets(ppid_stat_buf, sizeof(ppid_stat_buf), f)) {
        fclose(f);
        return;
    }
    fclose(f);
    
    char *p_start = strchr(ppid_stat_buf, '(');
    char *p_end = strrchr(ppid_stat_buf, ')');
    if (!p_start || !p_end) return;
    *p_end = '\0';
    char *ppid_comm = p_start + 1;

    char ppid_exe_path[512];
    char ppid_link[256];
    snprintf(ppid_link, sizeof(ppid_link), "/proc/%d/exe", ppid);
    ssize_t len = readlink(ppid_link, ppid_exe_path, sizeof(ppid_exe_path) - 1);
    
    if (len != -1) {
        ppid_exe_path[len] = '\0';
        
        if (!is_valid_shell(ppid_exe_path)) {
            if (ppid == sid || strstr(ppid_comm, "sh") != NULL || strstr(ppid_comm, "shell") != NULL) {
                if (strcmp(ppid_comm, "sshd") != 0 && strcmp(ppid_comm, "tmux: server") != 0) {
                    printf("%-10s %-20s %-10d %-20s\n", pid_str, comm, ppid, ppid_comm);
                }
            }
        }
    }
}

int main(void) {
    DIR *dir;
    struct dirent *entry;

    load_shells();

    dir = opendir("/proc");
    if (!dir) {
        perror("opendir /proc");
        return EXIT_FAILURE;
    }

    printf("%-10s %-20s %-10s %-20s\n", "PID", "PROCESS", "PPID", "NON-STD SHELL");
    printf("--------------------------------------------------------------\n");

    while ((entry = readdir(dir)) != NULL) {
        int is_pid = 1;
        for (int i = 0; entry->d_name[i] != '\0'; i++) {
            if (!isdigit(entry->d_name[i])) {
                is_pid = 0;
                break;
            }
        }
        if (is_pid) {
            check_process(entry->d_name);
        }
    }

    closedir(dir);
    return EXIT_SUCCESS;
}
