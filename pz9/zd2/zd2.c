#include <stdio.h>
#include <unistd.h>

int main() {
    execlp("sudo", "sudo", "cat", "/etc/shadow", NULL);
    return 1;
}
