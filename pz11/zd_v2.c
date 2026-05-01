#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sqlite3.h>

#define FIFO_PATH "/tmp/crash_fifo"

#define DB_PATH "crashes.db"

typedef struct {

    pid_t pid;

    int signo;

    void *fault_addr;

} crash_info_t;

void init_db(sqlite3 **db) {
    char *err_msg = 0;

    int rc = sqlite3_open(DB_PATH, db);

    if (rc != SQLITE_OK) {

        fprintf(stderr, "Cannot open database: %s\n", sqlite3_errmsg(*db));

        sqlite3_close(*db);

        exit(1);

    }

    const char *sql = "CREATE TABLE IF NOT EXISTS crashes("

                      "id INTEGER PRIMARY KEY AUTOINCREMENT, "

                      "pid INTEGER, "

                      "signo INTEGER, "

                      "fault_addr TEXT, "

                      "timestamp DATETIME DEFAULT CURRENT_TIMESTAMP);";

    rc = sqlite3_exec(*db, sql, 0, 0, &err_msg);

    if (rc != SQLITE_OK) {

        fprintf(stderr, "SQL error: %s\n", err_msg);

        sqlite3_free(err_msg);

    }

}



void run_daemon() {
    if (daemon(1, 0) == -1) {

        perror("daemon failed");

        exit(1);

    }

    sqlite3 *db;

    init_db(&db);

    mkfifo(FIFO_PATH, 0666); 

    while (1) {

        int fd = open(FIFO_PATH, O_RDONLY);

        if (fd == -1) continue;

        crash_info_t info;

        ssize_t bytes = read(fd, &info, sizeof(info));

        if (bytes == sizeof(info)) {

            char sql[256];

            snprintf(sql, sizeof(sql),

                     "INSERT INTO crashes (pid, signo, fault_addr) VALUES (%d, %d, '%p');",

                     info.pid, info.signo, info.fault_addr);

            sqlite3_exec(db, sql, 0, 0, 0);

        }

        close(fd);
    }
}

void crash_handler(int sig, siginfo_t *si, void *ctx) {
    (void)ctx;

    crash_info_t info;

    info.pid = getpid();

    info.signo = sig;

    info.fault_addr = si->si_addr;

    int fd = open(FIFO_PATH, O_WRONLY);

    if (fd != -1) {

        write(fd, &info, sizeof(info));

        close(fd);

    }

    _exit(128 + sig);
}

void setup_handlers() {

    struct sigaction sa;

    memset(&sa, 0, sizeof(sa));

    sa.sa_sigaction = crash_handler;

    sa.sa_flags = SA_SIGINFO | SA_RESETHAND;

    sigaction(SIGSEGV, &sa, NULL);

    sigaction(SIGFPE, &sa, NULL);

    sigaction(SIGILL, &sa, NULL);

}

int print_db_row(void *NotUsed, int argc, char **argv, char **azColName) {

    (void)NotUsed;

    for (int i = 0; i < argc; i++) {

        printf("%s: %s | ", azColName[i], argv[i] ? argv[i] : "NULL");

    }

    printf("\n");

    return 0;

}

int main(int argc, char *argv[]) {

    if (argc < 2) {

        printf("Usage:\n");

        printf("  %s daemon  - Start the background crash-monitor daemon\n", argv[0]);

        printf("  %s crash   - Run a process that intentionally crashes\n", argv[0]);

        printf("  %s read    - Show the SQLite database contents\n", argv[0]);

        return 1;

    }



    if (strcmp(argv[1], "daemon") == 0) {

        printf("Starting background daemon. It will silently log crashes to crashes.db...\n");

        run_daemon();

    } 

    else if (strcmp(argv[1], "crash") == 0) {

        setup_handlers();

        printf("Process %d is about to crash (causing SIGSEGV)...\n", getpid());

        volatile int *p = NULL;

        *p = 42;

    } 

    else if (strcmp(argv[1], "read") == 0) {

        sqlite3 *db;

        if (sqlite3_open(DB_PATH, &db) == SQLITE_OK) {

            printf("--- Database Records ---\n");

            sqlite3_exec(db, "SELECT * FROM crashes;", print_db_row, 0, 0);

            sqlite3_close(db);

        }
    } 

    else {

        printf("Unknown command.\n");

    }

    return 0;
}
