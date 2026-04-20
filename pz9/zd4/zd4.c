#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("Executing 'whoami' command:\n");
    system("whoami");
    
    printf("\nExecuting 'id' command to show user and group information:\n");
    system("id");
    
    return 0;
}
