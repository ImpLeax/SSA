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
        if (type == 0) {
            arr[i] = (rand() ^ (rand() << 15)) % size;
        } else if (type == 1) {
            arr[i] = i;
        } else if (type == 2) {
            arr[i] = size - i;
        } else if (type == 3) {
            arr[i] = 42;
        } else if (type == 4) {
            arr[i] = (i % 2 == 0) ? i : size - i;
        }
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
