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
