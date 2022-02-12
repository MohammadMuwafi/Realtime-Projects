#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

const int PRIME_NUMBER = 1e9 + 7;
const int ARRAY_SIZE = 10;
char arg[101];

/* This signal will call the workFunction. */
void signalHandler(int signalNumber);

/* This function will reset flag to let the player make his work as well as it
 * will call the array's generator. */
void doWork();

/* This function will generate an array with size [sz] and fills it with random
 * integers (1-100) and write it to file (either "child1.txt" or "child1.txt").
 */
void generateAndWriteArrayToFile(int sz);

/* Adding new line with fflushing the BUFFER. */
void newLine();

int flagToDoTheWork = 1;  // to control the signal.

/* this will take one argument which is the number of player (1 or 2)*/
int main(int argc, char* argv[]) {
    if (argc != 2) {
        perror("args should equal 2");
        exit(1);
    }
    /* Store the number of player. */
    strcpy(arg, argv[1]);

    if (sigset(10, signalHandler) == -1) {
        perror("Sigset cannot set SIGUSR1");
        exit(2);
    }
    while (1) {
        if (flagToDoTheWork) {
            signalHandler(SIGUSR1);
        }
        pause();
        // if (flag == 1) doWork();
    }
    return 0;
}

void signalHandler(int signalNumber) {
    if (signalNumber == 10) {
        // if the signal is SIGUSR1.
        flagToDoTheWork = 0;
        doWork();
    }
}

void doWork() {
    /* Generate a random array of size=10 and write it to the file. */
    int sz = 10;
    generateAndWriteArrayToFile(sz);
    flagToDoTheWork = 0;

    /* after finishing the work the player signals its father. */
    if (strcmp(arg, "1") == 0) {
        kill(getppid(), SIGINT);
    } else if (strcmp(arg, "2") == 0) {
        kill(getppid(), SIGQUIT);
    }
}

void generateAndWriteArrayToFile(int sz) {
    /* for seading. */
    srand((time(NULL) % getpid()) % PRIME_NUMBER);

    char str[101];
    if (strcmp(arg, "1") == 0) {
        strcpy(str, "child1.txt");
    } else if (strcmp(arg, "2") == 0) {
        strcpy(str, "child2.txt");
    } else {
        perror("Not as required");
        exit(2);
    }

    FILE* fp;
    fp = fopen(str, "w");
    if (fp == NULL) {
        perror("Error in opening file");
        exit(1);
    }

    for (int i = 0, val; i < 10; i++) {
        val = (rand() % 100) + 1;
        fprintf(fp, "%d\n", val);
    }
    newLine();
    fclose(fp);
}

void newLine() {
    printf("\n");
    fflush(stdout);
}
