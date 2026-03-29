#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/time.h>
#include "mymalloc.h"

void task1() {
    for (int i = 0; i < 120; i++) {
        char *p = malloc(1);
        free(p);
    }
}

void task2() {
    char *ptrs[120];
    for (int i = 0; i < 120; i++) {
        ptrs[i] = malloc(1);
    }
    for (int i = 0; i < 120; i++) {
        free(ptrs[i]);
    }
}

void task3() {
    char *ptrs[120];
    int count = 0;
    int allocated = 0;
    
    while (allocated < 120) {
        int choice = rand() % 2;
        if (choice == 0 || count == 0) {
            ptrs[count] = malloc(1);
            count++;
            allocated++;
        } else {
            int idx = rand() % count;
            free(ptrs[idx]);
            ptrs[idx] = ptrs[count - 1];
            count--;
        }
    }
    
    for (int i = 0; i < count; i++) {
        free(ptrs[i]);
    }
}

void task4() {
    char *list[50];
    for (int i = 0; i < 50; i++) {
        list[i] = malloc(10);
    }
    for (int i = 0; i < 50; i++) {
        if (i % 2 == 0) {
            free(list[i]);
        }
    }
    for (int i = 0; i < 50; i++) {
        if (i % 2 == 1) {
            free(list[i]);
        }
    }
}

void task5() {
    for (int i = 0; i < 60; i++) {
        char *x = malloc(20);
        char *y = malloc(30);
        free(x);
        free(y);
    }
}

int main(int argc, char **argv) {
    srand(time(NULL));
    
    struct timeval start, end;
    gettimeofday(&start, NULL);
    
    for (int i = 0; i < 50; i++) {
        task1();
        task2();
        task3();
        task4();
        task5();
    }
    
    gettimeofday(&end, NULL);
    
    long seconds = end.tv_sec - start.tv_sec;
    long micros = end.tv_usec - start.tv_usec;
    double elapsed = seconds + micros / 1000000.0;
    double average = elapsed / 50.0;
    
    printf("Average time per workload: %f seconds\n", average);
    
    return EXIT_SUCCESS;
}
