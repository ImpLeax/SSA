# Практична робота №8

## Тема: Системні виклики в UNIX/POSIX (файлові операції, процеси, міжпроцесна взаємодія)

**Мета роботи:** Вивчити базові системні виклики UNIX/POSIX на практиці. Дослідити поведінку файлових операцій при системних обмеженнях, роботу з покажчиками файлів, створення нових процесів та організацію міжпроцесної взаємодії (IPC) за допомогою неіменованих каналів (pipes).

---

## 1. Теоретичні відомості

Системні виклики (system calls) — це програмний інтерфейс, через який прикладні програми взаємодіють з ядром операційної системи. Вони дозволяють виконувати привілейовані операції: керувати пам'яттю, файлами, пристроями та процесами.

Для розуміння механізмів, використаних у роботі, наведено порівняльну таблицю ключових системних викликів:

### Таблиця 1. Огляд системних викликів POSIX

| Системний виклик | Категорія | Механізм роботи та особливості |
| :--- | :--- | :--- |
| `write()` / `read()` | Файловий ввід/вивід | Записують або зчитують певну кількість байтів. Можуть обробити менше байтів, ніж запитано (partial read/write) через переривання або ліміти. |
| `lseek()` | Файловий ввід/вивід | Змінює позицію покажчика читання/запису у відкритому файлі (дозволяє довільний доступ до байтів). |
| `fork()` | Керування процесами | Створює точну копію поточного процесу (дочірній процес). Повертає `0` дитині та `PID` дитини батьківському процесу. |
| `pipe()` | Взаємодія (IPC) | Створює однонаправлений канал даних у пам'яті (буфер), повертаючи два дескриптори: один для читання, інший для запису. |
| `dup2()` | Керування дескрипторами| Дублює файловий дескриптор. Часто використовується для підміни стандартного вводу/виводу (`stdin`/`stdout`) на `pipe`. |
| `execvp()` | Керування процесами | Замінює поточний образ процесу на нову програму. Якщо виклик успішний, керування до старого коду не повертається. |

---

## 2. Завдання 8.1: Частковий запис функцією write()

**Аналіз та механізм:** Системний виклик `count = write(fd, buffer, nbytes)` дійсно може повернути значення менше за `nbytes`. Це трапляється при нестачі місця на диску, перериванні сигналом або досягненні системного ліміту. У програмі навмисно встановлено жорсткий ліміт на розмір файлу (10 байтів) за допомогою функції `setrlimit`. При спробі записати 41 байт система зупиняє запис на 10-му байті.

### Програмна реалізація (`zd1.c`)
```c
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/resource.h>
#include <signal.h>

int main() {
    struct rlimit rl;
    int fd;
    char buffer[] = "This string contains more than ten bytes.";
    ssize_t bytes_written;

    rl.rlim_cur = 10;
    rl.rlim_max = 10;
    setrlimit(RLIMIT_FSIZE, &rl);

    signal(SIGXFSZ, SIG_IGN);

    fd = open("test_output.txt", O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        printf("Error opening file\n");
        return 1;
    }

    bytes_written = write(fd, buffer, 41);

    printf("Requested bytes to write: 41\n");
    printf("Actual bytes written: %zd\n", bytes_written);

    close(fd);
    return 0;
}
```

### Результат виконання
```text
impleax@Impleax:~/SSA/pz8/zd1$ ./zd1
Requested bytes to write: 41
Actual bytes written: 10
```

---

## 3. Завдання 8.2: Маніпуляції з покажчиком файлу (lseek)

**Аналіз та механізм:**
Файл містить байти `4, 5, 2, 2, 3, 3, 7, 9, 1, 5`. Виклик `lseek(fd, 3, SEEK_SET)` переміщує покажчик на позицію зі зміщенням 3 від початку файлу (індексація починається з нуля). Це означає, що покажчик стає на четвертий байт, який має значення `2`. Наступний виклик `read` зчитує 4 байти, починаючи з цієї позиції, тому в буфер потрапляють значення: `2, 3, 3, 7`.

### Програмна реалізація (`zd2.c`)
```c
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>

int main() {
    int fd;
    unsigned char initial_data[] = {4, 5, 2, 2, 3, 3, 7, 9, 1, 5};
    unsigned char buffer[4];
    int i;

    fd = open("data.bin", O_RDWR | O_CREAT | O_TRUNC, 0644);
    if (fd == -1) {
        printf("Error opening file\n");
        return 1;
    }

    write(fd, initial_data, sizeof(initial_data));

    lseek(fd, 3, SEEK_SET);

    read(fd, buffer, 4);

    printf("Buffer contents:\n");
    for (i = 0; i < 4; i++) {
        printf("%d ", buffer[i]);
    }
    printf("\n");

    close(fd);
    return 0;
}
```

### Результат виконання
```text
impleax@Impleax:~/SSA/pz8/zd2$ ./zd2
Buffer contents:
2 3 3 7
impleax@Impleax:~/SSA/pz8/zd2$ ls
data.bin  zd2  zd2.c
```

---

## 4. Завдання 8.3: Дослідження найгірших випадків qsort

**Аналіз та механізм:**
Класичний алгоритм QuickSort має найгіршу складність $O(N^2)$ при вже відсортованих, зворотно відсортованих масивах або масивах з однаковими елементами. Проте сучасні реалізації стандартної бібліотеки C (glibc) використовують вдосконалені алгоритми (наприклад, MergeSort або Introsort), які оптимізовані для таких випадків. Програма автоматично генерує масиви по 5 млн елементів різних типів для вимірювання швидкодії та включає тести коректності.

### Програмна реалізація (`zd3.c`)
```c
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <assert.h>

int cmp(const void *a, const void *b) {
    return (*(int*)a - *(int*)b);
}

int verify_sorted(int *arr, int n) {
    for (int i = 1; i < n; i++) {
        if (arr[i - 1] > arr[i]) return 0;
    }
    return 1;
}

void test_qsort_correctness() {
    int test1[] = {5, 2, 9, 1, 5, 6};
    int n1 = sizeof(test1) / sizeof(test1[0]);
    qsort(test1, n1, sizeof(int), cmp);
    assert(verify_sorted(test1, n1));

    int test2[] = {1, 2, 3, 4, 5};
    int n2 = sizeof(test2) / sizeof(test2[0]);
    qsort(test2, n2, sizeof(int), cmp);
    assert(verify_sorted(test2, n2));

    int test3[] = {9, 8, 7, 6};
    int n3 = sizeof(test3) / sizeof(test3[0]);
    qsort(test3, n3, sizeof(int), cmp);
    assert(verify_sorted(test3, n3));

    int test4[] = {4, 4, 4, 4};
    int n4 = sizeof(test4) / sizeof(test4[0]);
    qsort(test4, n4, sizeof(int), cmp);
    assert(verify_sorted(test4, n4));

    printf("All correctness tests passed.\n");
}

void run_experiment(int size, int type) {
    int *arr = (int *)malloc(size * sizeof(int));
    if (!arr) {
        printf("Memory allocation failed\n");
        return;
    }

    for (int i = 0; i < size; i++) {
        if (type == 0) arr[i] = (rand() ^ (rand() << 15)) % size;
        else if (type == 1) arr[i] = i;
        else if (type == 2) arr[i] = size - i;
        else if (type == 3) arr[i] = 42;
        else if (type == 4) arr[i] = (i % 2 == 0) ? i : size - i;
    }

    clock_t start = clock();
    qsort(arr, size, sizeof(int), cmp);
    clock_t end = clock();

    double time_spent = (double)(end - start) / CLOCKS_PER_SEC;

    const char* type_name = "";
    if (type == 0) type_name = "Random";
    else if (type == 1) type_name = "Sorted";
    else if (type == 2) type_name = "Reverse Sorted";
    else if (type == 3) type_name = "All Identical";
    else if (type == 4) type_name = "Alternating";

    printf("Type: %-15s | Size: %d | Time: %f seconds\n", type_name, size, time_spent);

    free(arr);
}

int main() {
    srand(time(NULL));

    printf("--- Running Correctness Tests ---\n");
    test_qsort_correctness();

    printf("\n--- Running Performance Experiments ---\n");
    int size = 5000000; 
    
    run_experiment(size, 0);
    run_experiment(size, 1);
    run_experiment(size, 2);
    run_experiment(size, 3);
    run_experiment(size, 4);

    return 0;
}
```

### Результат виконання
```text
impleax@Impleax:~/SSA/pz8/zd3$ ./zd3
--- Running Correctness Tests ---
All correctness tests passed.

--- Running Performance Experiments ---
Type: Random          | Size: 5000000 | Time: 0.548084 seconds
Type: Sorted          | Size: 5000000 | Time: 0.124425 seconds
Type: Reverse Sorted  | Size: 5000000 | Time: 0.150479 seconds
Type: All Identical   | Size: 5000000 | Time: 0.124384 seconds
Type: Alternating     | Size: 5000000 | Time: 0.193385 seconds
```

---

## 5. Завдання 8.4: Розгалуження процесів (fork)

**Аналіз та механізм:**
Виклик `fork()` успішно розщеплює програму на два паралельні процеси. Оскільки `printf("%d\n", pid);` знаходиться після `fork()`, він виконується обома процесами. Батьківський процес виводить PID створеної дитини (число, відмінне від нуля), а дочірній процес виводить `0`. При кожному новому запуску програми операційна система виділяє новий унікальний ідентифікатор для процесу-нащадка.

### Програмна реалізація (`zd4.c`)
```c
#include <stdio.h>
#include <unistd.h>

int main() {
    int pid;
    pid = fork();
    printf("%d\n", pid);
    return 0;
}
```

### Результат виконання
```text
impleax@Impleax:~/SSA/pz8/zd4$ ./zd4
1588
0
impleax@Impleax:~/SSA/pz8/zd4$ ./zd4
1590
0
impleax@Impleax:~/SSA/pz8/zd4$ ./zd4
1592
0
impleax@Impleax:~/SSA/pz8/zd4$ ./zd4
1594
0
```

---

## 6. Варіант 2: Інтерпретатор команд з підтримкою pipe

**Аналіз та механізм:**
У даному завданні реалізовано власну міні-оболонку (minishell), яка здатна обробляти конвеєри (pipes) між кількома командами.
1. Введений рядок розбивається на окремі команди за розділювачем `|`.
2. Для кожної команди у циклі створюється канал `pipe(fd)`.
3. Викликається `fork()`.
4. У дочірньому процесі (`pid == 0`) за допомогою `dup2()` стандартний вивід (`STDOUT`) перенаправляється у кінець запису поточного каналу `fd[1]`, а стандартний ввід (`STDIN`) береться з кінця читання попереднього каналу `prev_fd`. Після налаштування дескрипторів викликається `execvp()` для запуску системної утиліти.
5. Батьківський процес закриває непотрібні кінці каналів та чекає на завершення всіх дітей за допомогою `wait(NULL)`.

### Програмна реалізація (`zd_v2.c`)
```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>

#define MAX_INPUT 1024
#define MAX_ARGS 64
#define MAX_CMDS 16

void execute_pipeline(char *cmds[], int num_cmds) {
    int i;
    int fd[2];
    int prev_fd = 0;
    pid_t pid;

    for (i = 0; i < num_cmds; i++) {
        char *cmd_copy = strdup(cmds[i]);
        char *args[MAX_ARGS];
        int arg_count = 0;
        char *token = strtok(cmd_copy, " \t\n");

        while (token != NULL && arg_count < MAX_ARGS - 1) {
            args[arg_count++] = token;
            token = strtok(NULL, " \t\n");
        }
        args[arg_count] = NULL;

        if (arg_count == 0) {
            free(cmd_copy);
            continue;
        }

        if (i < num_cmds - 1) {
            if (pipe(fd) < 0) {
                perror("pipe failed");
                exit(1);
            }
        }

        pid = fork();

        if (pid < 0) {
            perror("fork failed");
            exit(1);
        } else if (pid == 0) {
            if (prev_fd != 0) {
                dup2(prev_fd, STDIN_FILENO);
                close(prev_fd);
            }
            if (i < num_cmds - 1) {
                dup2(fd[1], STDOUT_FILENO);
                close(fd[0]);
                close(fd[1]);
            }
            execvp(args[0], args);
            perror("execvp failed");
            exit(1);
        } else {
            if (prev_fd != 0) {
                close(prev_fd);
            }
            if (i < num_cmds - 1) {
                close(fd[1]);
                prev_fd = fd[0];
            }
            free(cmd_copy);
        }
    }

    for (i = 0; i < num_cmds; i++) {
        wait(NULL);
    }
}

int main() {
    char input[MAX_INPUT];
    char *cmds[MAX_CMDS];

    while (1) {
        printf("minishell> ");
        fflush(stdout);

        if (fgets(input, sizeof(input), stdin) == NULL) {
            break;
        }

        input[strcspn(input, "\n")] = 0;

        if (strcmp(input, "exit") == 0) {
            break;
        }

        int num_cmds = 0;
        char *cmd_token = strtok(input, "|");

        while (cmd_token != NULL && num_cmds < MAX_CMDS) {
            cmds[num_cmds++] = cmd_token;
            cmd_token = strtok(NULL, "|");
        }

        if (num_cmds > 0) {
            execute_pipeline(cmds, num_cmds);
        }
    }

    return 0;
}
```

### Результат виконання
```text
impleax@Impleax:~/SSA/pz8/zd_v2$ ./zd_v2
minishell> ls -l | grep task | wc -l
0
minishell> cat zd_v2.c | grep pipe
void execute_pipeline(char *cmds[], int num_cmds) {
            if (pipe(fd) < 0) {
                perror("pipe failed");
            execute_pipeline(cmds, num_cmds);
minishell> exit
```

---

## 7. Загальні висновки

1. **Системні виклики I/O** (такі як `write` та `read`) потребують перевірки результату виконання. Оскільки запит може бути оброблений частково, коректне системне програмування вимагає поміщати такі виклики у цикли для повної обробки даних.
2. **Керування покажчиками файлів (`lseek`)** є ключовим інструментом для парсингу бінарних даних, оскільки дозволяє гнучко переміщатися по структурі файлу без необхідності зчитувати його послідовно цілком.
3. **Оптимізація бібліотек:** Дослідження `qsort` підтвердило, що стандартні бібліотечні реалізації у Linux є високорівневими та захищеними від деградації до алгоритмічної складності $O(N^2)$ на специфічних (наприклад, вже відсортованих) даних.
4. **Конвеєрна взаємодія процесів:** Створення власного `minishell` наочно демонструє філософію UNIX, де складні задачі вирішуються комбінуванням простих утиліт. Використання `pipe` у поєднанні з `fork` та `dup2` є фундаментальним механізмом передачі даних між незалежними програмами на рівні ОС.
