# Практична робота №11

## Тема: Розробка демонів, обробка аварійних сигналів та взаємодія з базами даних SQLite у Linux

**Мета роботи:** Опанувати принципи створення фонових процесів (демонів) у UNIX-подібних операційних системах. Реалізувати міжпроцесну взаємодію за допомогою іменованих каналів (FIFO) для перехоплення фатальних сигналів (SIGSEGV, SIGFPE, SIGILL) та навчитися працювати з вбудованою базою даних SQLite на рівні C-API для збереження контексту аварій.

---

## 1. Теоретичні відомості

У Linux демони — це фонові процеси, які від'єднані від керуючого терміналу і працюють непомітно для користувача. Вони часто використовуються для виконання системних завдань, моніторингу або обробки запитів.

Аварійні завершення процесів зазвичай супроводжуються генерацією фатальних сигналів, таких як `SIGSEGV` (помилка доступу до пам'яті), `SIGFPE` (арифметична помилка, наприклад, ділення на нуль) або `SIGILL` (недійсна інструкція). Щоб зафіксувати ці події до того, як процес буде остаточно знищено системою, використовуються спеціальні обробники сигналів (наприклад, через `sigaction`), які зберігають мінімально необхідний контекст.

Для передачі інформації від аварійного процесу до фонового демона ефективно використовувати іменовані канали (FIFO). Вони працюють як спеціальні файли, куди один процес пише дані, а інший звідти їх читає. Для збереження та структурування отриманої діагностичної інформації (PID, номер сигналу, адреса збою) оптимально підходить SQLite — компактна реляційна база даних, що зберігає всі дані в одному файлі.

---

## 2. Підготовка середовища (Встановлення бібліотек)

Для роботи з базою даних SQLite на рівні мови C необхідно встановити відповідні бібліотеки для розробки (`libsqlite3-dev`) та саму утиліту `sqlite3`. 

**Виконані команди в терміналі:**
```bash
sudo apt update
sudo apt install libsqlite3-dev sqlite3
```
*Ці команди оновили список пакетів та встановили необхідні залежності для успішної компіляції C-коду з директивою `#include <sqlite3.h>`.*

---

## 3. Реалізація завдання

### 3.1. Завдання за варіантом (Варіант 2)
**Опис:** Створіть демон, який відстежує процеси, що аварійно завершуються (через SIGFPE, SIGILL, SIGSEGV) та зберігає всю інформацію про контекст у базу даних SQLite. 

Для виконання завдання було розроблено єдину програму-утиліту, яка залежно від переданого аргументу може працювати в трьох режимах:
1. **`daemon`** — запускається у фоновому режимі, створює БД і слухає FIFO-канал.
2. **`crash`** — ініціалізує обробник сигналів, навмисно викликає `SIGSEGV` та відправляє свій контекст у FIFO.
3. **`read`** — читає базу даних SQLite та виводить логі аварій у консоль.

**Код програми (`zd_v2.c`):**
```c
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
    sqlite3_exec(*db, sql, 0, 0, &err_msg);
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
    if (argc < 2) return 1;

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
    return 0;
}
```

**Вивід у термінал:**
```text
impleax@Impleax:~/SSA/pz11$ sudo apt install libsqlite3-dev sqlite3
... [скорочено лог встановлення] ...
impleax@Impleax:~/SSA/pz11$ vim zd_v2.c
impleax@Impleax:~/SSA/pz11$ gcc -Wall -Wextra zd_v2.c -o zd_v2 -lsqlite3
impleax@Impleax:~/SSA/pz11$ ./zd_v2 daemon
Starting background daemon. It will silently log crashes to crashes.db...
impleax@Impleax:~/SSA/pz11$ ./zd_v2 crash
Process 2056 is about to crash (causing SIGSEGV)...
impleax@Impleax:~/SSA/pz11$ ./zd_v2 read
--- Database Records ---
id: 1 | pid: 2056 | signo: 11 | fault_addr: (nil) | timestamp: 2026-05-01 09:45:58 |
impleax@Impleax:~/SSA/pz11$ pkill zd_v2
```

**Аналіз результатів:** 
1. Програму успішно скомпільовано з прив'язкою бібліотеки SQLite (`-lsqlite3`). 
2. При виклику команди `./zd_v2 daemon` процес успішно від'єднався від терміналу завдяки функції `daemon()`, створивши локальну базу даних `crashes.db` та іменований канал `/tmp/crash_fifo`. 
3. Наступним кроком було запущено тестовий процес `./zd_v2 crash`, який отримав ідентифікатор `2056` і навмисне звернувся до нульового вказівника. Обробник `crash_handler` перехопив сигнал, сформував пакет даних і відправив його через FIFO до фонового демона.
4. Читання бази даних (`./zd_v2 read`) підтвердило, що демон успішно отримав дані та зберіг їх в SQLite: зафіксовано PID `2056`, номер сигналу `11` (відповідає `SIGSEGV`), адресу збою та точний час аварії.
5. Наприкінці життєвий цикл фонового процесу було коректно завершено системною утилітою `pkill`.

---

## 4. Висновки

Під час виконання одинадцятої практичної роботи було успішно досліджено архітектуру фонових програм-демонів та механізми їхньої роботи. Практично реалізовано моніторинговий сервіс, який здатен незалежно від основного терміналу працювати в системі та збирати діагностичні дані. Завдяки використанню іменованих каналів (FIFO) було налаштовано надійний та блискавичний канал зв'язку (IPC) між падаючим процесом та демоном, що гарантує збереження інформації навіть під час фатальних аварій пам'яті. Крім того, закріплено навички інтеграції вбудованої бази даних SQLite у C-застосунки, що дозволило структурувати логування контексту системних збоїв.
