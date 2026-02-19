#include <stdio.h>


int arr[1000] = {1};

int main() {
	int arr_local[1000];

	int arr_local2[1000] = {1};

	arr_local[0] = 5;
	printf("hello world %d, %d", arr_local[0], arr_local2[0]);
	return 0;
}
