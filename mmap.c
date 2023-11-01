// Final program that amalgamates all the three page replacement algorithms
// Written by Vedant Kumbhar    - 
//            Mithil Pechimuthu - 21110129

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(int argc, char *argv[]) {
    int N = atoi(argv[1]);

    printf("Running LRU Page Replacement Algorithm...\n");
    char lru_command[256];
    sprintf(lru_command, "gcc LRU.c -lm -o lru; ./lru %d", N);
    system(lru_command);

    printf("\nRunning FIFO Page Replacement Algorithm...\n");
    char fifo_command[256];
    sprintf(fifo_command, "gcc FIFO.c -lm -o fifo; ./fifo %d", N);
    system(fifo_command);

    printf("\nRunning Clock Page Replacement Algorithm...\n");
    char clock_command[256];
    sprintf(clock_command, "gcc ClockAlgo.c -lm -o clock; ./clock %d", N);
    system(clock_command);
    return 0;
}
// #include <stdio.h>
// #include <stdlib.h>
// #include <string.h>
// #include <unistd.h>

// int main(int argc, char *argv[]) {
//     int N = atoi(argv[1]);

//     // Check if the binaries exist, and compile them if they don't
//     if (access("lru", F_OK) == -1) {
//         printf("Compiling LRU Page Replacement Algorithm...\n");
//         if (system("gcc LRU.c -o lru -lm") != 0) {
//             fprintf(stderr, "Compilation of LRU failed.\n");
//             return 1;
//         }
//     }

//     if (access("fifo", F_OK) == -1) {
//         printf("Compiling FIFO Page Replacement Algorithm...\n");
//         if (system("gcc FIFO.c -o fifo -lm") != 0) {
//             fprintf(stderr, "Compilation of FIFO failed.\n");
//             return 1;
//         }
//     }

//     if (access("clock", F_OK) == -1) {
//         printf("Compiling Clock Page Replacement Algorithm...\n");
//         if (system("gcc ClockAlgo.c -o clock -lm") != 0) {
//             fprintf(stderr, "Compilation of Clock failed.\n");
//             return 1;
//         }
//     }

//     printf("Running LRU Page Replacement Algorithm...\n");
//     char lru_command[256];
//     sprintf(lru_command, "./lru %d", N);
//     system(lru_command);

//     printf("\nRunning FIFO Page Replacement Algorithm...\n");
//     char fifo_command[256];
//     sprintf(fifo_command, "./fifo %d", N);
//     system(fifo_command);

//     printf("\nRunning Clock Page Replacement Algorithm...\n");
//     char clock_command[256];
//     sprintf(clock_command, "./clock %d", N);
//     system(clock_command);
//     return 0;
// }
