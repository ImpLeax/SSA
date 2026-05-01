# Практична робота №10

## Тема: Сигнальна взаємодія, обробка аварійних завершень та контекст процесів у Linux

**Мета роботи:** Опанувати механізми обробки фатальних сигналів та отримання діагностичної інформації при аваріях (register dumping). Дослідити правильні підходи до використання затримок часу при перериванні сигналами, а також реалізувати міжпроцесну взаємодію за допомогою сигналів реального часу та системного виклику `fork`.

---

## 1. Теоретичні відомості

Сигнал у Linux — це механізм асинхронного сповіщення процесу або потоку про певну подію. Ця подія може бути зовнішньою (наприклад, відправка команди `kill`) або внутрішньою (наприклад, некоректне звернення до пам'яті, що породжує `SIGSEGV`). У випадку фатальних сигналів стан процесу може бути небезпечним: пошкоджений стек або заблоковані м'ютекси. Тому правильна стратегія (graceful crash handling) полягає не у продовженні роботи, а у фіксації діагностики та контрольованому завершенні процесу. 

Для обробки таких аварій використовується функція `sigaction()` з прапором `SA_SIGINFO`, що дозволяє отримати контекст виконання та зберегти адресу пам'яті, де сталася помилка. 

Крім того, сигнали впливають на системні виклики очікування (наприклад, `nanosleep`). Доставка сигналу може перервати сон процесу, повернувши помилку `EINTR`. Щоб уникнути накопичення похибки в періодичних задачах (drift), використовують абсолютні таймери за допомогою `clock_nanosleep()`. Також розширені можливості надають POSIX-сигнали реального часу (real-time signals), які стають у чергу і дозволяють передавати між процесами корисне навантаження.

---

## 2. Реалізація загальних завдань

### 2.1. Завдання 10.1: Перехоплення аварійного завершення та діагностика пам'яті
**Опис:** Розроблено програму, яка навмисно викликає `SIGSEGV` (через звернення до нульового вказівника) і перехоплює його. Обробник сигналу виводить адресу помилки та дамп регістрів (RIP, RSP тощо), після чого за допомогою утиліти `addr2line` виконується пошук рядка коду, що спричинив збій.

**Код (`crash_diag.c`):**
```c
#define _GNU_SOURCE
#include <errno.h>
#include <signal.h>
#include <stdint.h>
#include <string.h>
#include <sys/ucontext.h>
#include <ucontext.h>
#include <unistd.h>
#include <stdlib.h>

static void wr_all(const char *s, unsigned long n) {
    while (n > 0) {
        ssize_t r = write(STDERR_FILENO, s, n);
        if (r <= 0) return;
        s += r;
        n -= (unsigned long)r;
    }
}

static void wr(const char *s) {
    unsigned long n = 0;
    while (s[n] != '\0') n++;
    wr_all(s, n);
}

static void wr_ch(char c) {
    wr_all(&c, 1);
}

static void wr_dec(long v) {
    char buf[32];
    int i = 0;
    unsigned long x;
    if (v < 0) {
        wr_ch('-');
        x = (unsigned long)(-(v + 1)) + 1UL;
    } else {
        x = (unsigned long)v;
    }
    do {
        buf[i++] = (char)('0' + (x % 10));
        x /= 10;
    } while (x != 0 && i < (int)sizeof(buf));
    while (i > 0) wr_ch(buf[--i]);
}

static void wr_hex(uint64_t v) {
    static const char hex[] = "0123456789abcdef";
    int started = 0;
    wr("0x");
    for (int shift = 60; shift >= 0; shift -= 4) {
        unsigned int nib = (unsigned int)((v >> shift) & 0xfU);
        if (nib != 0 || started || shift == 0) {
            wr_ch(hex[nib]);
            started = 1;
        }
    }
}

static void wr_ptr(const void *p) {
    wr_hex((uint64_t)(uintptr_t)p);
}

static void crash_handler(int sig, siginfo_t *si, void *ctx) {
    int saved_errno = errno;
    wr("\n=== crash captured ===\n");
    wr("signal: ");
    wr_dec(sig);
    wr("\n");
    if (si != NULL) {
        wr("si_code: ");
        wr_dec((long)si->si_code);
        wr("\n");
        wr("fault address: ");
        wr_ptr(si->si_addr);
        wr("\n");
    }
#if defined(__x86_64__)
    if (ctx != NULL) {
        ucontext_t *uc = (ucontext_t *)ctx;
        greg_t *g = uc->uc_mcontext.gregs;
        wr("RIP: "); wr_hex((uint64_t)g[REG_RIP]); wr("\n");
        wr("RSP: "); wr_hex((uint64_t)g[REG_RSP]); wr("\n");
        wr("RBP: "); wr_hex((uint64_t)g[REG_RBP]); wr("\n");
        wr("RAX: "); wr_hex((uint64_t)g[REG_RAX]); wr("\n");
        wr("RBX: "); wr_hex((uint64_t)g[REG_RBX]); wr("\n");
        wr("RCX: "); wr_hex((uint64_t)g[REG_RCX]); wr("\n");
        wr("RDX: "); wr_hex((uint64_t)g[REG_RDX]); wr("\n");
        wr("RSI: "); wr_hex((uint64_t)g[REG_RSI]); wr("\n");
        wr("RDI: "); wr_hex((uint64_t)g[REG_RDI]); wr("\n");
    }
#else
    wr("Register dump is implemented here only for x86-64.\n");
#endif
    errno = saved_errno;
    exit(128 + sig);
}

static void install_crash_handlers(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_sigaction = crash_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_SIGINFO | SA_RESETHAND;
    sigaction(SIGSEGV, &sa, NULL);
    sigaction(SIGBUS, &sa, NULL);
    sigaction(SIGFPE, &sa, NULL);
    sigaction(SIGILL, &sa, NULL);
    sigaction(SIGABRT, &sa, NULL);
}

__attribute__((noinline))
static void crash_here(void) {
    volatile int *p = (int *)0;
    *p = 42;
}

int main(void) {
    install_crash_handlers();
    wr("About to crash. PID=");
    wr_dec((long)getpid());
    wr("\n");
    crash_here();
    return 0;
}
```

**Вивід у термінал:**
```text
impleax@Impleax:~/SSA/pz10/zd1$ gcc -Wall -Wextra -O0 -g -fno-omit-frame-pointer -no-pie crash_diag.c -o crash_diag
impleax@Impleax:~/SSA/pz10/zd1$ ./crash_diag
About to crash. PID=1258

=== crash captured ===
signal: 11
si_code: 1
fault address: 0x0
RIP: 0x401818
RSP: 0x7ffc267548d0
RBP: 0x7ffc267548d0
RAX: 0x0
RBX: 0x7ffc26754a08
RCX: 0x781e9ff1c5a4
RDX: 0x1
RSI: 0x402035
RDI: 0x2
impleax@Impleax:~/SSA/pz10/zd1$ addr2line -e ./crash_diag -f -C 0x401818
crash_here
/home/impleax/SSA/pz10/zd1/crash_diag.c:115
```
**Аналіз:** Програма успішно перехопила сигнал `11` (`SIGSEGV`). Структура `siginfo_t` дозволила витягнути fault address (`0x0`). Завдяки збереженню регістрів (`ucontext`) було отримано адресу інструкції (`RIP: 0x401818`). Використання `addr2line` точно вказало, що збій виник у функції `crash_here` на 115-му рядку вихідного файлу.

---

### 2.2. Завдання 10.2: Коректне використання sleep при перериванні сигналами
**Опис:** Програма демонструє обробку помилки `EINTR` під час використання відносного сну (`nanosleep`) та використання абсолютних дедлайнів (`clock_nanosleep`) для уникнення накопичення похибки у періодичних задачах.

**Код (`sleep_correct.c`):**
```c
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

static volatile sig_atomic_t got_usr1 = 0;

static void on_usr1(int sig) {
    (void)sig;
    got_usr1 = 1;
}

static int sleep_relative_ms(long ms) {
    struct timespec req = {
        .tv_sec = ms / 1000,
        .tv_nsec = (ms % 1000) * 1000000L
    };
    struct timespec rem;
    while (nanosleep(&req, &rem) == -1) {
        if (errno == EINTR) {
            req = rem;
            continue;
        }
        return -1;
    }
    return 0;
}

static void add_ms(struct timespec *t, long ms) {
    t->tv_sec += ms / 1000;
    t->tv_nsec += (ms % 1000) * 1000000L;
    while (t->tv_nsec >= 1000000000L) {
        t->tv_sec++;
        t->tv_nsec -= 1000000000L;
    }
}

static int sleep_periodic_absolute(struct timespec *deadline, long period_ms) {
    int rc;
    add_ms(deadline, period_ms);
    while ((rc = clock_nanosleep(CLOCK_MONOTONIC, TIMER_ABSTIME, deadline, NULL)) == EINTR) {
    }
    if (rc != 0) {
        errno = rc;
        return -1;
    }
    return 0;
}

int main(void) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = on_usr1;
    sigemptyset(&sa.sa_mask);
    if (sigaction(SIGUSR1, &sa, NULL) == -1) {
        perror("sigaction");
        return 1;
    }

    printf("PID=%ld. In another terminal: kill -USR1 %ld\n", (long)getpid(), (long)getpid());
    puts("Relative sleep for 5 seconds using nanosleep restart loop...");

    if (sleep_relative_ms(5000) == -1) {
        perror("nanosleep");
        return 1;
    }

    printf("Relative sleep finished. got_usr1=%d\n", got_usr1);
    puts("Now 5 periodic ticks with absolute clock_nanosleep deadlines...");

    struct timespec next;
    if (clock_gettime(CLOCK_MONOTONIC, &next) == -1) {
        perror("clock_gettime");
        return 1;
    }

    for (int i = 1; i <= 5; i++) {
        if (sleep_periodic_absolute(&next, 1000) == -1) {
            perror("clock_nanosleep");
            return 1;
        }
        printf("tick %d\n", i);
    }
    return 0;
}
```

**Вивід у термінал:**
```text
impleax@Impleax:~/SSA/pz10/zd2$ ./sleep_correct
PID=1512. In another terminal: kill -USR1 1512
Relative sleep for 5 seconds using nanosleep restart loop...
Relative sleep finished. got_usr1=1
Now 5 periodic ticks with absolute clock_nanosleep deadlines...
tick 1
tick 2
tick 3
tick 4
tick 5
```
**Аналіз:** Проведено кілька запусків програми. У випадку надсилання сигналу `kill -USR1` з іншого терміналу, прапорець `got_usr1` змінив значення на `1`. Незважаючи на переривання, цикл `while` перевірив `EINTR` і дозволив програмі доспати залишок часу, а також успішно завершити 5 рівномірних тіків без зміщення графіку.

---

### 2.3. Завдання 10.3: IPC за допомогою сигналів реального часу
**Опис:** Демонстрація архітектури Publisher-Subscriber. "Видавець" надсилає сигнали з корисним навантаженням через `sigqueue()`, а "підписник" їх приймає за допомогою синхронного очікування `sigwaitinfo()`.

**Код (`rt_pubsub.c`):**
```c
#define _POSIX_C_SOURCE 200809L
#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

static void die(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

static long parse_long(const char *s, const char *what) {
    char *end = NULL;
    errno = 0;
    long v = strtol(s, &end, 10);
    if (errno != 0 || end == s || *end != '\0') {
        fprintf(stderr, "invalid %s: %s\n", what, s);
        exit(EXIT_FAILURE);
    }
    return v;
}

static int app_signal(void) {
    int sig = SIGRTMIN;
    if (sig > SIGRTMAX) {
        fprintf(stderr, "No available real-time signal\n");
        exit(EXIT_FAILURE);
    }
    return sig;
}

static void usage(const char *prog) {
    fprintf(stderr,
            "Usage:\n"
            "  %s sub\n"
            "  %s sub-timeout\n"
            "  %s pub <subscriber-pid> <int> [<int>...]\n"
            "Example:\n"
            "  terminal 1: %s sub\n"
            "  terminal 2: %s pub <pid> 10 20 30 -1\n",
            prog, prog, prog, prog, prog);
}

static void subscriber(int use_timeout) {
    int sig = app_signal();
    sigset_t set;
    sigemptyset(&set);
    sigaddset(&set, sig);

    if (sigprocmask(SIG_BLOCK, &set, NULL) == -1) {
        die("sigprocmask");
    }

    printf("subscriber PID=%ld, waiting for signal %d (SIGRTMIN)\n", (long)getpid(), sig);
    fflush(stdout);

    for (;;) {
        siginfo_t si;
        memset(&si, 0, sizeof(si));
        int r;

        if (use_timeout) {
            struct timespec ts = {
                .tv_sec = 5,
                .tv_nsec = 0
            };
            r = sigtimedwait(&set, &si, &ts);
        } else {
            r = sigwaitinfo(&set, &si);
        }

        if (r == -1) {
            if (errno == EINTR) {
                continue;
            }
            if (use_timeout && errno == EAGAIN) {
                puts("timeout: no messages for 5 seconds");
                continue;
            }
            die(use_timeout ? "sigtimedwait" : "sigwaitinfo");
        }

        int value = si.si_value.sival_int;
        printf("received signal=%d value=%d from pid=%ld uid=%ld\n",
               sig, value, (long)si.si_pid, (long)si.si_uid);
        fflush(stdout);

        if (value < 0) {
            puts("negative value received: shutting down subscriber");
            break;
        }
    }
}

static void publisher(pid_t pid, int argc, char **argv) {
    int sig = app_signal();
    for (int i = 3; i < argc; i++) {
        union sigval value;
        value.sival_int = (int)parse_long(argv[i], "message value");
        if (sigqueue(pid, sig, value) == -1) {
            die("sigqueue");
        }
        printf("sent value=%d to pid=%ld via signal=%d\n", value.sival_int, (long)pid, sig);
    }
}

int main(int argc, char **argv) {
    if (argc < 2) {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    if (strcmp(argv[1], "sub") == 0) {
        subscriber(0);
    } else if (strcmp(argv[1], "sub-timeout") == 0) {
        subscriber(1);
    } else if (strcmp(argv[1], "pub") == 0) {
        if (argc < 4) {
            usage(argv[0]);
            return EXIT_FAILURE;
        }
        pid_t pid = (pid_t)parse_long(argv[2], "PID");
        publisher(pid, argc, argv);
    } else {
        usage(argv[0]);
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
```

**Вивід у термінал:**
```text
impleax@Impleax:~/SSA/pz10/zd3$ ./rt_pubsub sub
subscriber PID=1538, waiting for signal 34 (SIGRTMIN)
received signal=34 value=10 from pid=1539 uid=1000
received signal=34 value=20 from pid=1539 uid=1000
received signal=34 value=30 from pid=1539 uid=1000
received signal=34 value=-1 from pid=1539 uid=1000
negative value received: shutting down subscriber
```
**Аналіз:** Програма успішно прийняла кілька сигналів поспіль. На відміну від звичайних сигналів, які можуть "зливатися" в один, сигнали реального часу були оброблені по черзі завдяки черзі. Дані (10, 20, 30, -1) були передані з процесу `1539` до процесу `1538` за допомогою структури `union sigval`. Отримавши `-1`, підписник штатно завершив роботу.

---

## 3. Завдання за варіантом (Варіант 2)

### 3.1. Виведення ідентифікаторів родинних процесів
**Опис завдання:** Напишіть програму, в якій дочірній процес виводить свій PID та PID батьківського процесу, а потім завершується.

**Код (`zd_v2.c`):**
```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

int main() {
    pid_t pid = fork();

    if (pid < 0) {
        perror("Fork failed");
        exit(EXIT_FAILURE);
    }
    else if (pid == 0) {
        printf("=== Child Process ===\n");
        printf("My PID (Child): %d\n", getpid());
        printf("My Parent's PID: %d\n", getppid());
        exit(EXIT_SUCCESS);
    }
    else {
        wait(NULL);
        printf("=== Parent Process ===\n");
        printf("Child process finished successfully. Parent (PID: %d) is also exiting.\n", getpid());
    }

    return 0;
}
```

**Вивід у термінал:**
```text
impleax@Impleax:~/SSA/pz10/zd_v2$ gcc -Wall zd_v2.c -o zd_v2
impleax@Impleax:~/SSA/pz10/zd_v2$ ./zd_v2
=== Child Process ===
My PID (Child): 1553
My Parent's PID: 1552
=== Parent Process ===
Child process finished successfully. Parent (PID: 1552) is also exiting.
```
**Аналіз:** Програма успішно розгалузила виконання за допомогою системного виклику `fork()`. Блок коду з умовою `if (pid == 0)` відпрацював у дочірньому процесі, успішно отримавши свій ідентифікатор (`1553`) та ідентифікатор процесу-творця (`1552`) за допомогою `getpid()` та `getppid()`. Завдяки `wait(NULL)` батьківський процес (1552) синхронно дочекався завершення "дитини", перш ніж вивести своє фінальне повідомлення.

---

## 4. Висновки

Під час виконання десятої практичної роботи було глибоко досліджено механізми роботи із сигналами та керування процесами в Linux. Було успішно перевірено роботу обробників фатальних сигналів з вилученням низькорівневої діагностичної інформації з контексту процесора. На практиці закріплено розуміння відмінностей між відносними та абсолютними таймерами під час переривань, а також побудовано найпростіший IPC-канал на основі сигналів реального часу. Індивідуальне завдання дозволило закріпити знання системного виклику `fork()` та базової ієрархії (Parent-Child) процесів у UNIX-подібних системах.
