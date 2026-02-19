#include <stdio.h>
#include <stdlib.h>

int global_init = 10;       
int global_uninit;          

void grow_stack() {
    int large_array[10000]; 
    int local_var;          
    
    
    large_array[0] = 1;     
    
    printf("The new stack top is near: %p\n", (void*)&local_var);
}

int main() {
    int i;                               
    int *heap_var = malloc(sizeof(int)); 

    printf("--- Memory Segments Addresses ---\n");
    printf("Text segment (main function):  %p\n", (void*)main);
    printf("Data segment (global_init):    %p\n", (void*)&global_init);
    printf("BSS segment (global_uninit):   %p\n", (void*)&global_uninit);
    printf("Heap segment (malloc):         %p\n", (void*)heap_var);
    printf("The stack top in main is near: %p\n", (void*)&i);
    
    grow_stack();

    free(heap_var);
    return 0;
}
