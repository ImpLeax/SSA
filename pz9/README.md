# Практична робота №9

## Тема: Дослідження системи прав доступу, управління обліковими записами та контексту виконання процесів у Linux

**Мета роботи:** Опанувати механізми управління правами доступу до файлів та каталогів у Linux. Дослідити взаємодію між звичайними користувачами та суперкористувачем (root), роботу з системними файлами облікових записів (`/etc/passwd`, `/etc/shadow`), а також реалізувати програмне керування привілеями, правами доступу та обходом стандартних обмежень оболонки.

---

## 1. Теоретичні відомості

У UNIX/Linux-файлових системах, щоб запустити файл як програму або скрипт, необхідно, щоб були виставлені права на виконання. Однак існує виняток: якщо скрипт не має прав на виконання, але має права на читання, його можна передати як аргумент до інтерпретатора напряму.

Тут відбувається не запуск скрипта безпосередньо, а виклик інтерпретатора (наприклад, `bash`), який читає вміст файлу та виконує його. Таким чином, обхід прав на виконання — можливий. 

Права на виконання потрібні лише для прямого запуску скрипта (`./script.sh`). Але якщо вказується інтерпретатор (`bash script.sh`, `python script.py`), потрібні лише права на читання. Це часто використовується в автоматизації, у тестуванні безпеки для обходу обмежень та при аналізі прав доступу.

---

## 2. Реалізація загальних завдань

### 2.1. Завдання 9.1: Аналіз облікових записів системи
**Опис:** Розроблено програму, яка читає вивід команди `getent passwd` для пошуку звичайних користувачів (UID >= 1000), виключаючи поточного користувача.

**Код (`zd1.c`):**
```c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>

int main() {
    FILE *fp;
    char line[1024];
    uid_t my_uid = getuid();
    int found_others = 0;

    fp = popen("getent passwd", "r");
    if (fp == NULL) return 1;

    printf("Current User UID: %d\n", my_uid);
    printf("Searching for other regular users (UID >= 1000)...\n\n");

    while (fgets(line, sizeof(line), fp) != NULL) {
        char *username = strtok(line, ":");
        strtok(NULL, ":"); 
        char *uid_str = strtok(NULL, ":");
        
        if (uid_str != NULL) {
            int uid = atoi(uid_str);
            if (uid >= 1000 && uid < 65534 && uid != my_uid) {
                printf("Found user: %s (UID: %d)\n", username, uid);
                found_others = 1;
            }
        }
    }

    if (!found_others) printf("No other regular users found on this system.\n");

    pclose(fp);
    return 0;
}
```

**Вивід у термінал:**
```text
impleax@Impleax:~/SSA/pz9/zd1$ ./zd1
Current User UID: 1000
Searching for other regular users (UID >= 1000)...

No other regular users found on this system.
```
**Аналіз:** Програма успішно використовує `popen` для читання системних даних. Оскільки в системі WSL налаштовано лише одного основного користувача `impleax`, програма коректно повідомила про відсутність інших звичайних користувачів.

---

### 2.2. Завдання 9.2: Ескалація привілеїв для читання /etc/shadow
**Опис:** Програма виконує команду читання захищеного файлу `/etc/shadow` від імені адміністратора, маючи права звичайного користувача.

**Код (`zd2.c`):**
```c
#include <stdio.h>
#include <unistd.h>

int main() {
    execlp("sudo", "sudo", "cat", "/etc/shadow", NULL);
    return 1;
}
```

**Вивід у термінал:**
```text
impleax@Impleax:~/SSA/pz9/zd2$ ./zd2
[sudo] password for impleax:
root:*:20094:0:99999:7:::
daemon:*:20094:0:99999:7:::
bin:*:20094:0:99999:7:::
sys:*:20094:0:99999:7:::
...
impleax:$y$j9T$DVQH03irVJk01GkLcnT0B.$UwyJhdY6iekgsQMgA8uciFMYJKS2VAsuWq8o6CfSl51:20482:0:99999:7:::
```
**Аналіз:** Використано системний виклик `execlp` для заміни поточного процесу утилітою `sudo`. Це дозволяє тимчасово отримати права `root` (після введення пароля) і прочитати файл з хешами паролів.

---

### 2.3. Завдання 9.3: Парадокси прав доступу та видалення файлів
**Опис:** Моделювання ситуації, коли файл належить root, але знаходиться в домашньому каталозі користувача.

**Код (`zd3.c`):**
```c
#include <stdio.h>
#include <stdlib.h>

int main() {
    system("echo 'Initial content' > my_temp_file.txt");
    printf("Copying file as root...\n");
    system("sudo cp my_temp_file.txt $HOME/root_copied_file.txt");
    
    printf("\nAttempting to modify the file as a regular user...\n");
    system("echo 'Appended content' >> $HOME/root_copied_file.txt");
    
    printf("\nAttempting to delete the file as a regular user...\n");
    system("rm $HOME/root_copied_file.txt");
    system("rm -f my_temp_file.txt");
    return 0;
}
```

**Вивід у термінал:**
```text
impleax@Impleax:~/SSA/pz9/zd3$ ./zd3
Copying file as root...

Attempting to modify the file as a regular user...
sh: 1: cannot create /home/impleax/root_copied_file.txt: Permission denied

Attempting to delete the file as a regular user...
rm: remove write-protected regular file '/home/impleax/root_copied_file.txt'? y
```
**Аналіз:** Спроба зміни файлу блокується (`Permission denied`), оскільки власником копії є `root`. Проте видалення файлу проходить успішно, оскільки право на видалення залежить від прав доступу до *батьківського каталогу* (домашнього каталогу `impleax`), а не самого файлу.

---

### 2.4. Завдання 9.4: Перевірка контексту користувача та груп
**Опис:** Виконання команд `whoami` та `id` для отримання повної інформації про поточного користувача.

**Код (`zd4.c`):**
```c
#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("Executing 'whoami' command:\n");
    system("whoami");
    printf("\nExecuting 'id' command to show user and group information:\n");
    system("id");
    return 0;
}
```

**Вивід у термінал:**
```text
impleax@Impleax:~/SSA/pz9/zd4$ ./zd4
Executing 'whoami' command:
impleax

Executing 'id' command to show user and group information:
uid=1000(impleax) gid=1000(impleax) groups=1000(impleax),4(adm),20(dialout),24(cdrom),25(floppy),27(sudo),29(audio),30(dip),44(video),46(plugdev),100(users),107(netdev),1001(docker)
```
**Аналіз:** Демонструється, що користувач належить не лише до своєї основної групи (`1000`), а й до низки додаткових системних груп, таких як `sudo` (права адміністратора), `docker` тощо.

---

### 2.5. Завдання 9.5: Дослідження впливу chmod та chown
**Опис:** Програма створює файл, передає його у власність `root` і змінює права (600, 644, 666), аналізуючи можливість доступу за допомогою функції `access()`.

**Код (`zd5.c`):**
```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void check_access(const char *filename) {
    printf("Read access: ");
    if (access(filename, R_OK) == 0) printf("YES\t"); else printf("NO\t");
    printf("Write access: ");
    if (access(filename, W_OK) == 0) printf("YES\n"); else printf("NO\n");
}

int main() {
    const char *filename = "test_temp_file.txt";
    FILE *fp = fopen(filename, "w");
    if (fp != NULL) { fputs("Test data\n", fp); fclose(fp); }
    
    printf("Changing file owner to root...\n");
    system("sudo chown root test_temp_file.txt");
    
    printf("\nSetting permissions to 600 (root: rw, others: none)...\n");
    system("sudo chmod 600 test_temp_file.txt");
    check_access(filename);
    
    printf("\nSetting permissions to 644 (root: rw, others: read-only)...\n");
    system("sudo chmod 644 test_temp_file.txt");
    check_access(filename);
    
    printf("\nSetting permissions to 666 (root: rw, others: read-write)...\n");
    system("sudo chmod 666 test_temp_file.txt");
    check_access(filename);
    
    system("sudo rm -f test_temp_file.txt");
    return 0;
}
```

**Вивід у термінал:**
```text
impleax@Impleax:~/SSA/pz9/zd5$ ./zd5
Changing file owner to root...

Setting permissions to 600 (root: rw, others: none)...
Read access: NO Write access: NO

Setting permissions to 644 (root: rw, others: read-only)...
Read access: YES        Write access: NO

Setting permissions to 666 (root: rw, others: read-write)...
Read access: YES        Write access: YES
```
**Аналіз:** Оскільки власником стає `root`, звичайний користувач отримує права з категорії "others" (остання цифра). Тестування `access()` чітко підтвердило математику прав доступу: 0 (відмовлено), 4 (тільки читання), 6 (читання та запис).

---

### 2.6. Завдання 9.6: Аналіз системних файлів та спроби їх обходу
**Опис:** Програма намагається прочитати, записати, виконати та змінити права доступу на файлах із різних каталогів: `$HOME`, `/etc` та `/usr/bin`.

**Код (`zd6.c`):**
```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

void test_file_access(const char *path) {
    char cmd[256];
    FILE *f;

    printf("--- Inspecting: %s ---\n", path);
    snprintf(cmd, sizeof(cmd), "ls -ld %s", path);
    system(cmd);

    printf("Attempting READ: ");
    f = fopen(path, "r");
    if (f) { printf("SUCCESS\n"); fclose(f); } else printf("DENIED\n");

    printf("Attempting WRITE: ");
    f = fopen(path, "a");
    if (f) { printf("SUCCESS\n"); fclose(f); } else printf("DENIED\n");

    printf("Attempting EXECUTE: ");
    if (access(path, X_OK) == 0) printf("SUCCESS\n"); else printf("DENIED\n");

    printf("Attempting BYPASS (change permissions): ");
    snprintf(cmd, sizeof(cmd), "chmod 777 %s 2>/dev/null", path);
    if (system(cmd) == 0) printf("SUCCESS\n"); else printf("DENIED\n");
    printf("\n");
}

int main() {
    char home_path[256];
    snprintf(home_path, sizeof(home_path), "%s/my_test_file.txt", getenv("HOME"));
    char create_cmd[256];
    snprintf(create_cmd, sizeof(create_cmd), "touch %s", home_path);
    system(create_cmd);

    test_file_access(home_path);
    test_file_access("/etc/passwd");
    test_file_access("/etc/shadow");
    test_file_access("/usr/bin/whoami");

    char rm_cmd[256];
    snprintf(rm_cmd, sizeof(rm_cmd), "rm -f %s", home_path);
    system(rm_cmd);

    return 0;
}
```

**Вивід у термінал:**
```text
impleax@Impleax:~/SSA/pz9/zd6$ ./zd6
--- Inspecting: /home/impleax/my_test_file.txt ---
-rw-r--r-- 1 impleax impleax 0 Apr 20 09:40 /home/impleax/my_test_file.txt
Attempting READ: SUCCESS
Attempting WRITE: SUCCESS
Attempting EXECUTE: DENIED
Attempting BYPASS (change permissions): SUCCESS

--- Inspecting: /etc/passwd ---
-rw-r--r-- 1 root root 1481 Apr 13 08:17 /etc/passwd
Attempting READ: SUCCESS
Attempting WRITE: DENIED
Attempting EXECUTE: DENIED
Attempting BYPASS (change permissions): DENIED

--- Inspecting: /etc/shadow ---
-rw-r----- 1 root shadow 823 Apr 13 08:17 /etc/shadow
Attempting READ: DENIED
Attempting WRITE: DENIED
Attempting EXECUTE: DENIED
Attempting BYPASS (change permissions): DENIED

--- Inspecting: /usr/bin/whoami ---
-rwxr-xr-x 1 root root 35336 Jun 22  2025 /usr/bin/whoami
Attempting READ: SUCCESS
Attempting WRITE: DENIED
Attempting EXECUTE: SUCCESS
Attempting BYPASS (change permissions): DENIED
```
**Аналіз:** Ядро Linux чітко виконує розмежування прав: власний файл можна модифікувати як завгодно, `/etc/passwd` відкритий лише для читання, `/etc/shadow` недоступний взагалі, а виконувані файли в `/usr/bin` доступні лише для запуску та читання, але захищені від запису та несанкціонованої зміни атрибутів.

---

## 3. Завдання за варіантом (Варіант 2)

### 3.1. Динамічне обмеження доступу за часом
**Опис завдання:** Створити систему, де обмежується доступ до певного файлу лише в певні години. 
Для вирішення цього завдання було написано програму (демон), яка моніторить поточний час і за допомогою функції `chmod()` відкриває або закриває доступ до файлу.

**Код (`zd_v2.c`):**
```c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <sys/stat.h>

int main(int argc, char *argv[]) {
    if (argc != 4) return 1;

    const char *filename = argv[1];
    int start_hour = atoi(argv[2]);
    int end_hour = atoi(argv[3]);

    FILE *f = fopen(filename, "a");
    if (f) { fputs("Protected content.\n", f); fclose(f); }

    printf("Monitoring file: %s\n", filename);
    printf("Allowed window: %02d:00 - %02d:00\n", start_hour, end_hour);
    printf("Press Ctrl+C to stop the monitor.\n\n");

    while (1) {
        time_t t = time(NULL);
        struct tm *tm_info = localtime(&t);
        int current_hour = tm_info->tm_hour;

        if (current_hour >= start_hour && current_hour < end_hour) {
            chmod(filename, 0600);
            printf("[%02d:%02d:%02d] STATUS: UNLOCKED (Permissions: 600)\n", 
                   tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
        } else {
            chmod(filename, 0000);
            printf("[%02d:%02d:%02d] STATUS: LOCKED (Permissions: 000)\n", 
                   tm_info->tm_hour, tm_info->tm_min, tm_info->tm_sec);
        }
        sleep(10);
    }
    return 0;
}
```

**Вивід у термінал:**
```text
impleax@Impleax:~/SSA/pz9/zd_v2$ ./zd_v2 secret.txt 10 15
Monitoring file: secret.txt
Allowed window: 10:00 - 15:00
Press Ctrl+C to stop the monitor.

[09:42:29] STATUS: LOCKED (Permissions: 000)
^C
impleax@Impleax:~/SSA/pz9/zd_v2$ cat secret.txt
cat: secret.txt: Permission denied

impleax@Impleax:~/SSA/pz9/zd_v2$ ./zd_v2 secret.txt 8 12
Monitoring file: secret.txt
Allowed window: 08:00 - 12:00
Press Ctrl+C to stop the monitor.

[09:44:19] STATUS: UNLOCKED (Permissions: 600)
^C
impleax@Impleax:~/SSA/pz9/zd_v2$ cat secret.txt
Protected content.
```
**Аналіз:** Програма успішно реалізує концепцію "Time-Based Access Control". Використовуючи системні виклики часу (`time`, `localtime`) та керування правами (`chmod 0000` / `chmod 0600`), програма перекриває доступ на рівні файлової системи залежно від часу. В термінальному тестуванні підтверджено, що в заборонений час доступ до файлу блокується (`Permission denied`).

---

## 4. Висновки

Під час виконання лабораторної роботи було детально досліджено модель безпеки ОС Linux. Практично перевірено роботу команд керування привілеями (`su`, `sudo`), вивчено структуру файлів `/etc/passwd` та `/etc/shadow`. Найважливішим результатом стало розуміння тонкощів прав доступу: продемонстровано, що право на видалення файлу залежить від прав на каталог, а не на сам файл. Завдяки індивідуальному завданню було розроблено ефективний механізм контролю доступу за часовими інтервалами за допомогою системних викликів `chmod()`, що ілюструє гнучкість Linux у побудові нестандартних політик безпеки на рівні мови С.
