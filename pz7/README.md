# Практична робота №7

## Тема: Дослідження, моделювання та нестандартні підходи до аналізу процесів, файлових систем, безпеки та ресурсів в Linux

**Мета роботи:** Опанувати методи системного програмування в середовищі Linux. Дослідити механізми взаємодії з ядром операційної системи, використання системних викликів (syscalls), роботу з файловими дескрипторами, правами доступу, а також реалізувати парсинг системної інформації через віртуальну файлову систему `/proc` та управління процесами.

---

## 1. Теоретичні відомості

Системне програмування в ОС Linux вимагає прямої взаємодії з апаратними ресурсами та низькорівневими сервісами, оминаючи високорівневі утиліти (наприклад, `ps`, `top`, `ls`). Важливим аспектом є моделювання поведінки системи. 

Зокрема, моделювання черги задач (Job Queue) дозволяє організувати відкладене або послідовне виконання функцій. У середовищах, де заборонено використання багатопоточності або системних сигналів (наприклад, `SIGSTOP`, `SIGCONT`), управління станами задач (`PENDING`, `RUNNING`, `PAUSED`, `COMPLETED`) реалізується через єдиний потік виконання за допомогою циклічної перевірки масиву структур. Це вимагає жорсткої синхронізації логіки всередині головного циклу програми.

---

## 2. Реалізація загальних завдань

### 2.1. Задача 1: Взаємодія процесів через `popen()`
**Опис:** Реалізовано передачу виводу команди `rwho` до команди `more` за допомогою каналів (pipes). 

**Код (`zd1.c`):**
```c
#include <stdio.h>
#include <stdlib.h>

int main(void) {
    FILE *rwho_pipe;
    FILE *more_pipe;
    char buffer[1024];

    rwho_pipe = popen("rwho", "r");
    if (rwho_pipe == NULL) {
        perror("Error opening rwho pipe");
        exit(EXIT_FAILURE);
    }

    more_pipe = popen("more", "w");
    if (more_pipe == NULL) {
        perror("Error opening more pipe");
        pclose(rwho_pipe);
        exit(EXIT_FAILURE);
    }

    while (fgets(buffer, sizeof(buffer), rwho_pipe) != NULL) {
        fputs(buffer, more_pipe);
    }

    pclose(rwho_pipe);
    pclose(more_pipe);

    return 0;
}
```

**Вивід у термінал:**
```text
impleax@Impleax:~/SSA/pz7/zd1$ sudo systemctl start rwhod
impleax@Impleax:~/SSA/pz7/zd1$ sudo ./zd1
impleax  Impleax:pts/1 Apr 13 08:10 :43
```
**Аналіз:** Використання функції `popen()` дозволяє створити односпрямований канал між програмою та процесом оболонки. Дані успішно зчитуються з `rwho` і перенаправляються у потік запису `more`.

---

### 2.2. Задача 2: Імітація утиліти `ls -l`
**Опис:** Розроблено програму, яка зчитує вміст поточного каталогу та виводить детальну інформацію про файли (права доступу, кількість посилань, власник, група, розмір, час модифікації), використовуючи системні структури.

**Код (`zd2.c`):**
```c
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
        if (entry->d_name[0] == '.') continue;
        if (stat(entry->d_name, &file_stat) == -1) continue;

        print_permissions(file_stat.st_mode);
        printf("%lu ", (unsigned long)file_stat.st_nlink);

        pwd = getpwuid(file_stat.st_uid);
        if (pwd != NULL) printf("%s ", pwd->pw_name);
        else printf("%d ", file_stat.st_uid);

        grp = getgrgid(file_stat.st_gid);
        if (grp != NULL) printf("%s ", grp->gr_name);
        else printf("%d ", file_stat.st_gid);

        printf("%5ld ", (long)file_stat.st_size);

        struct tm *tm_info = localtime(&file_stat.st_mtime);
        strftime(time_str, sizeof(time_str), "%b %d %H:%M", tm_info);
        printf("%s %s\n", time_str, entry->d_name);
    }

    closedir(dir);
    return EXIT_SUCCESS;
}
```

**Вивід у термінал:**
```text
impleax@Impleax:~/SSA/pz7/zd2$ ./zd2
-rw-r--r-- 1 impleax impleax  1879 Apr 13 08:21 zd2.c
-rwxr-xr-x 1 impleax impleax 16536 Apr 13 08:22 zd2
impleax@Impleax:~/SSA/pz7/zd2$ ls -l
total 24
-rwxr-xr-x 1 impleax impleax 16536 Apr 13 08:22 zd2
-rw-r--r-- 1 impleax impleax  1879 Apr 13 08:21 zd2.c
```
**Аналіз:** Програма успішно відтворює поведінку `ls -l`. Використання функцій `stat()`, `getpwuid()` та `getgrgid()` дозволило коректно отримати метадані файлів без виклику зовнішніх утиліт.

---

### 2.3. Задача 3: Спрощена версія `grep`
**Опис:** Реалізовано пошук заданого слова у текстовому файлі з виведенням відповідних рядків.

**Код (`zd3.c`):**
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_LINE_LENGTH 2048

int main(int argc, char *argv[]) {
    FILE *file;
    char line[MAX_LINE_LENGTH];

    if (argc != 3) {
        fprintf(stderr, "Usage: %s <word> <file>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    file = fopen(argv[2], "r");
    if (file == NULL) {
        perror("Error opening file");
        exit(EXIT_FAILURE);
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        if (strstr(line, argv[1]) != NULL) {
            printf("%s", line);
        }
    }

    fclose(file);
    return EXIT_SUCCESS;
}
```

**Вивід у термінал:**
```text
impleax@Impleax:~/SSA/pz7/zd3$ ./zd3 apple test.txt
apple tree
apple pie
```
**Аналіз:** Базовий пошук підрядка ефективно виконується функцією `strstr()`. Програма успішно обробляє передані аргументи командного рядка.

---

### 2.4. Задача 4: Спрощена версія `more`
**Опис:** Створено утиліту для посторінкового (по 20 рядків) перегляду текстових файлів із зупинкою та очікуванням натискання клавіші.

**Код (`zd4.c`):**
```c
#include <stdio.h>
#include <stdlib.h>
#include <termios.h>
#include <unistd.h>

#define LINES_PER_PAGE 20
#define MAX_LINE_LENGTH 2048

int wait_for_keypress(void) {
    struct termios old_term, new_term;
    int ch;
    FILE *tty;

    tty = fopen("/dev/tty", "r+");
    if (tty == NULL) return -1;

    tcgetattr(fileno(tty), &old_term);
    new_term = old_term;
    new_term.c_lflag &= ~(ICANON | ECHO);
    tcsetattr(fileno(tty), TCSANOW, &new_term);

    ch = fgetc(tty);

    tcsetattr(fileno(tty), TCSANOW, &old_term);
    fclose(tty);

    return ch;
}

void process_file(const char *filename) {
    FILE *file = fopen(filename, "r");
    char line[MAX_LINE_LENGTH];
    int line_count = 0, key;

    if (file == NULL) {
        perror(filename);
        return;
    }

    while (fgets(line, sizeof(line), file) != NULL) {
        printf("%s", line);
        line_count++;

        if (line_count == LINES_PER_PAGE) {
            key = wait_for_keypress();
            if (key == 'q' || key == 'Q') {
                fclose(file);
                exit(EXIT_SUCCESS);
            }
            line_count = 0;
        }
    }
    fclose(file);
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        fprintf(stderr, "Usage: %s <file1> [file2 ...]\n", argv[0]);
        return EXIT_FAILURE;
    }
    for (int i = 1; i < argc; i++) process_file(argv[i]);
    return EXIT_SUCCESS;
}
```

**Вивід у термінал:**
```text
impleax@Impleax:~/SSA/pz7/zd4$ ./zd4 test_more.txt
This is line number 1
...
This is line number 20
...
This is line number 50
```
**Аналіз:** Використання структури `termios` дозволило перевести термінал у неканонічний режим, що дає змогу зчитувати натискання клавіш без необхідності натискати `Enter`. Робота ведеться безпосередньо з пристроєм `/dev/tty`.

---

### 2.5. Задача 5: Рекурсивний обхід каталогів
**Опис:** Програма рекурсивно проходить по всіх підкаталогах і виводить шляхи до знайдених файлів.

**Код (`zd5.c`):**
```c
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

    if (dir == NULL) return;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;

        snprintf(path, sizeof(path), "%s/%s", base_path, entry->d_name);
        printf("%s\n", path);

        if (stat(path, &statbuf) == 0 && S_ISDIR(statbuf.st_mode)) {
            list_directory(path);
        }
    }
    closedir(dir);
}

int main(void) {
    list_directory(".");
    return EXIT_SUCCESS;
}
```

**Вивід у термінал:**
```text
impleax@Impleax:~/SSA/pz7/zd5$ ./zd5
./zd5.c
./zd5
```
**Аналіз:** Обхід реалізовано шляхом рекурсивного виклику функції `list_directory()` при зустрічі об'єктів з атрибутом `S_ISDIR`.

---

### 2.6. Задача 6: Вивід підкаталогів у алфавітному порядку
**Опис:** Реалізовано фільтрацію вмісту каталогу для виведення виключно підкаталогів, відсортованих за алфавітом.

**Код (`zd6.c`):**
```c
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <string.h>

int is_dir(const struct dirent *entry) {
    struct stat statbuf;
    if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) return 0;
    if (stat(entry->d_name, &statbuf) == 0 && S_ISDIR(statbuf.st_mode)) return 1;
    return 0;
}

int main(void) {
    struct dirent **namelist;
    int n = scandir(".", &namelist, is_dir, alphasort);

    if (n < 0) {
        perror("scandir");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < n; i++) {
        printf("%s\n", namelist[i]->d_name);
        free(namelist[i]);
    }
    free(namelist);
    return EXIT_SUCCESS;
}
```

**Вивід у термінал:**
```text
impleax@Impleax:~/SSA/pz7/zd6$ ./zd6
A_folder
M_folder
Z_folder
```
**Аналіз:** Використання системної функції `scandir()` разом із вбудованим компаратором `alphasort` є найбільш оптимальним рішенням для сортування директорій на рівні мови С.

---

### 2.7. Задача 7: Інтерактивне управління правами доступу
**Опис:** Програма шукає файли з розширенням `.c`, власником яких є поточний користувач, і пропонує додати права на читання для інших користувачів.

**Код (`zd7.c`):**
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void) {
    DIR *dir = opendir(".");
    struct dirent *entry;
    struct stat file_stat;
    uid_t my_uid = getuid();
    char response[16];

    if (dir == NULL) return EXIT_FAILURE;

    while ((entry = readdir(dir)) != NULL) {
        size_t len = strlen(entry->d_name);
        if (len > 2 && entry->d_name[len - 2] == '.' && entry->d_name[len - 1] == 'c') {
            if (stat(entry->d_name, &file_stat) == 0 && S_ISREG(file_stat.st_mode) && file_stat.st_uid == my_uid) {
                printf("Found your C file: %s\n", entry->d_name);
                printf("Grant read permission to others? (y/n): ");

                if (fgets(response, sizeof(response), stdin) != NULL) {
                    if (response[0] == 'y' || response[0] == 'Y') {
                        mode_t new_mode = file_stat.st_mode | S_IROTH;
                        if (chmod(entry->d_name, new_mode) == 0) {
                            printf("Read permission granted for %s\n\n", entry->d_name);
                        } else perror("chmod failed");
                    } else printf("Skipped %s\n\n", entry->d_name);
                }
            }
        }
    }
    closedir(dir);
    return EXIT_SUCCESS;
}
```

**Вивід у термінал:**
```text
impleax@Impleax:~/SSA/pz7/zd7$ chmod o-r *.c
impleax@Impleax:~/SSA/pz7/zd7$ ls -l *.c
-rw-r----- 1 impleax impleax 1627 Apr 13 08:33 zd7.c
impleax@Impleax:~/SSA/pz7/zd7$ ./zd7
Found your C file: zd7.c
Grant read permission to others? (y/n): y
Read permission granted for zd7.c
impleax@Impleax:~/SSA/pz7/zd7$ ls -l *.c
-rw-r--r-- 1 impleax impleax 1627 Apr 13 08:33 zd7.c
```
**Аналіз:** Використано виклик `chmod()` із побітовим додаванням прапорця `S_IROTH` для модифікації прав доступу на льоту.

---

### 2.8. Задача 8: Інтерактивне видалення файлів
**Опис:** Утиліта запитує підтвердження на видалення кожного файлу у поточному каталозі.

**Код (`zd8.c`):**
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <unistd.h>

int main(void) {
    DIR *dir = opendir(".");
    struct dirent *entry;
    struct stat file_stat;
    char response[16];

    if (dir == NULL) return EXIT_FAILURE;

    while ((entry = readdir(dir)) != NULL) {
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) continue;
        if (stat(entry->d_name, &file_stat) == 0 && S_ISREG(file_stat.st_mode)) {
            printf("Delete file '%s'? (y/n): ", entry->d_name);
            if (fgets(response, sizeof(response), stdin) != NULL) {
                if (response[0] == 'y' || response[0] == 'Y') {
                    if (unlink(entry->d_name) == 0) printf("Deleted '%s'\n", entry->d_name);
                    else perror("unlink failed");
                } else printf("Skipped '%s'\n", entry->d_name);
            }
        }
    }
    closedir(dir);
    return EXIT_SUCCESS;
}
```

**Вивід у термінал:**
```text
impleax@Impleax:~/SSA/pz7/zd8/test_delete$ ./zd8
Delete file 'zd8'? (y/n): y
Deleted 'zd8'
Delete file 'my_data.csv'? (y/n): n
Skipped 'my_data.csv'
Delete file 'test2.log'? (y/n): y
Deleted 'test2.log'
...
```
**Аналіз:** Видалення файлу реалізовано через системний виклик `unlink()`, який видаляє посилання на файл (inode).

---

### 2.9. Задача 9: Вимірювання часу виконання
**Опис:** Вимірюється час роботи циклу у мілісекундах за допомогою `gettimeofday()`.

**Код (`zd9.c`):**
```c
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

void target_code(void) {
    volatile long long sum = 0;
    for (long long i = 0; i < 500000000LL; i++) sum += i;
}

int main(void) {
    struct timeval start, end;
    long long elapsed_ms;

    gettimeofday(&start, NULL);
    target_code();
    gettimeofday(&end, NULL);

    elapsed_ms = ((end.tv_sec - start.tv_sec) * 1000LL) + ((end.tv_usec - start.tv_usec) / 1000LL);
    printf("Execution time: %lld milliseconds\n", elapsed_ms);

    return EXIT_SUCCESS;
}
```

**Вивід у термінал:**
```text
impleax@Impleax:~/SSA/pz7/zd9$ ./zd9
Execution time: 200 milliseconds
```
**Аналіз:** Функція `gettimeofday()` забезпечує мікросекундну точність. Використання кваліфікатора `volatile` для змінної `sum` запобігло оптимізації циклу компілятором.

---

### 2.10. Задача 10: Генерація випадкових чисел з плаваючою точкою
**Опис:** Розроблено програму, яка генерує дві послідовності псевдовипадкових чисел з плаваючою точкою: перша у базовому діапазоні від 0.0 до 1.0, друга — у діапазоні від 0.0 до заданого числа `n` (значення передається як аргумент командного рядка; якщо аргумент відсутній, за замовчуванням використовується 100.0).

**Код (`zd10.c`):**
```c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int main(int argc, char *argv[]) {
    int i;
    double n;

    srand((unsigned int)time(NULL));

    printf("Sequence (0.0 to 1.0):\n");
    for (i = 0; i < 5; i++) {
        printf("%f\n", (double)rand() / RAND_MAX);
    }

    if (argc > 1) {
        n = atof(argv[1]);
    } else {
        n = 100.0;
    }

    printf("\nSequence (0.0 to %f):\n", n);
    for (i = 0; i < 5; i++) {
        printf("%f\n", ((double)rand() / RAND_MAX) * n);
    }

    return EXIT_SUCCESS;
}
```

**Вивід у термінал:**
```text
impleax@Impleax:~/SSA/pz7/zd10$ ./zd10
Sequence (0.0 to 1.0):
0.762634
0.077159
0.179622
0.412593
0.597592

Sequence (0.0 to 100.000000):
88.705187
79.121207
58.456416
51.766804
3.922470
impleax@Impleax:~/SSA/pz7/zd10$ ./zd10 5.5
Sequence (0.0 to 1.0):
0.552704
0.640140
0.690301
0.010950
0.764916

Sequence (0.0 to 5.500000):
5.434465
3.265351
3.060049
2.977860
2.526982
impleax@Impleax:~/SSA/pz7/zd10$ ./zd10 5.5
Sequence (0.0 to 1.0):
0.552704
0.640140
0.690301
0.010950
0.764916

Sequence (0.0 to 5.500000):
5.434465
3.265351
3.060049
2.977860
2.526982
```
**Аналіз:** Отримання дробових значень реалізовано шляхом ділення цілочисельного результату функції `rand()` на константу `RAND_MAX` із попереднім приведенням до типу `double`. Масштабування до заданого діапазону виконується простим множенням базового результату на `n`. Ініціалізація генератора (seed) забезпечується викликом `srand((unsigned int)time(NULL));`. 

У наведеному термінальному виводі чітко зафіксовано важливу особливість цього класичного методу ініціалізації: під час двох послідовних запусків команди `./zd10 5.5` було згенеровано абсолютно ідентичні послідовності чисел. Це пояснюється тим, що функція `time(NULL)` повертає системний час із точністю до однієї секунди. Оскільки обидва запуски програми відбулися дуже швидко (в межах тієї ж секунди), генератор псевдовипадкових чисел отримав однакове початкове значення (seed), що й призвело до дублювання результату.

## 3. Завдання за варіантом (Варіант 2)

### 3.1. Виявлення процесів з нестандартних оболонок
**Опис завдання:** Реалізувати утиліту командного рядка, яка виводить процеси, запущені лише з нестандартних шеллів, оминаючи стандартні утиліти моніторингу (`ps`, `top`). 

**Код (`zd_v2.c`):**
```c
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
    if (!fgets(stat_buf, sizeof(stat_buf), f)) { fclose(f); return; }
    fclose(f);

    char *start = strchr(stat_buf, '(');
    char *end = strrchr(stat_buf, ')');
    if (!start || !end) return;

    *start = '\0'; *end = '\0';
    char *comm = start + 1;

    int ppid, pgrp, sid, tty_nr;
    if (sscanf(end + 2, "%*c %d %d %d %d", &ppid, &pgrp, &sid, &tty_nr) != 4) return;
    if (tty_nr == 0 || ppid == 0) return;

    char ppid_stat_path[512], ppid_stat_buf[1024];
    snprintf(ppid_stat_path, sizeof(ppid_stat_path), "/proc/%d/stat", ppid);
    f = fopen(ppid_stat_path, "r");
    if (!f) return;
    if (!fgets(ppid_stat_buf, sizeof(ppid_stat_buf), f)) { fclose(f); return; }
    fclose(f);

    char *p_start = strchr(ppid_stat_buf, '(');
    char *p_end = strrchr(ppid_stat_buf, ')');
    if (!p_start || !p_end) return;
    *p_end = '\0';
    char *ppid_comm = p_start + 1;

    char ppid_link[256], ppid_exe_path[512];
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
    if (!dir) return EXIT_FAILURE;

    printf("%-10s %-20s %-10s %-20s\n", "PID", "PROCESS", "PPID", "NON-STD SHELL");
    printf("--------------------------------------------------------------\n");

    while ((entry = readdir(dir)) != NULL) {
        int is_pid = 1;
        for (int i = 0; entry->d_name[i] != '\0'; i++) {
            if (!isdigit(entry->d_name[i])) { is_pid = 0; break; }
        }
        if (is_pid) check_process(entry->d_name);
    }
    closedir(dir);
    return EXIT_SUCCESS;
}
```

**Вивід у термінал:**
```text
impleax@Impleax:~/SSA/pz7/zd_v2$ cp /bin/bash ./mysh
impleax@Impleax:~/SSA/pz7/zd_v2$ ./mysh
impleax@Impleax:~/SSA/pz7/zd_v2$ sleep 300 &
[1] 2223
impleax@Impleax:~/SSA/pz7/zd_v2$ ./zd_v2
PID        PROCESS              PPID       NON-STD SHELL
--------------------------------------------------------------
2223       sleep                2081       mysh
2224       zd_v2                2081       mysh
```
**Аналіз:** Для реалізації завдання було обрано метод парсингу псевдофайлової системи `/proc`. Спочатку програма зчитує системний список легітимних оболонок з `/etc/shells`. Далі програма ітерує по всіх числових директоріях у `/proc` (які відповідають PID процесів). За допомогою зчитування `/proc/[PID]/stat` визначається батьківський процес (PPID). Використання системного виклику `readlink` на `/proc/[PPID]/exe` дозволяє отримати реальний абсолютний шлях до виконуваного файлу оболонки. Під час тестування було створено копію оболонки (`./mysh`), і програма успішно відслідкувала процеси `sleep` та `zd_v2`, запущені з неї.

---

## 4. Висновки

Під час виконання лабораторної роботи було практично досліджено інструменти системного програмування. Реалізовано аналоги базових утиліт UNIX (`ls`, `grep`, `more`), що дозволило поглибити розуміння роботи з файловими дескрипторами та метаданими. Найважливішим етапом стала розробка утиліти моніторингу за варіантом, яка довела, що зчитування та парсинг даних з `/proc` дозволяє отримувати вичерпну інформацію про стани процесів, їхніх предків та виконувані бінарні файли без залучення стороннього програмного забезпечення.
