#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <time.h>
#include <unistd.h>

#define NUMBER_OF_CHILDREN 3 /* The number of children. */
#define STR_LEN 51           /* The default size of strings. */
#define MAX_SZ 101           /* The default length of string. */
#define ARRAY_SIZE 10        /* Size of array to be generate. */

/* The sig_hndlr will counts the children that signal the parent. */
void signalHandler(int signalNumber);

/* Make another round of guessing. */
void makeAnotherGuess();

/* Update scores for each players after the current round. */
void updateScores(int cnt1, int cnt2);

/* Apply the rules of the game here such that check if someone wins and the game
 * is finished or not. */
void isFinished();

/* The parent will send an info to referee to check the results. */
void sendToReferee();

/* Referee will open the files for each player and sends the result to father
 * and deletes the files. */
void refereeWork();

/* The program should be terminated when the game finished and this function
 * will kill all children as well as the program itself. */
void killWholeProgram();

/* The BUFFER should be always empty so after printing on terminal, it is
 * important to clear BUFFER using fflush(stdout). */
void newLine();

/* This function will wait for n seconds but with loop to be smoothy in
 * terminal. */
void delay(int n);

/* Close all of file descriptors that were opened for pipes. */
void closeDesc();

/* Print the random array for each player. */
void printArray(int* arr, int n, int p);

/* Print the round number in good format. */
void printRoundNumber();

/* Count the number of digits of number.*/
int countDigits(int n);

int numOfProcesses = 0;          /* Counter for processes that signal parent. */
int totalScore1 = 0;             /* Total score of Player1 in till now. */
int totalScore2 = 0;             /* Total score of Player2 in till now. */
int numOfRounds = 0;             /* Number of round till now. */
int cnt1 = 0;                    /* Score of Player1 in a match*/
int cnt2 = 0;                    /* Score of Player2 in a match*/
int fd1[2];                      /* Parent Writes and R reads. */
int fd2[2];                      /* R Writes and Parent reads. */
int childID[NUMBER_OF_CHILDREN]; /* IDs for children. */

int main(int argc, char* argv[]) {
    printf("\033[37m");

    if (pipe(fd1) == -1 || pipe(fd2) == -1) {
        perror("Error in creating pipe");
        exit(1);
    }

    if (sigset(SIGINT, signalHandler) == -1 ||
        sigset(SIGQUIT, signalHandler) == -1) {
        perror("Sigset cannot set SIGQUIT or SIGINT");
        exit(2);
    }

    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        pid_t pid = fork();
        childID[i] = pid;
        if (pid == 0 && i == 0 &&
            (execlp("./child", "./child", "1", NULL) == -1)) {
            perror("Cannot execute P1");
            exit(1);
        } else if (pid == 0 && i == 1 &&
                   (execlp("./child", "./child", "2", NULL) == -1)) {
            perror("Cannot execute P2");
            exit(2);
        } else if (pid == 0 && i == 2) {
            while (1) {
                refereeWork();
                // pause();
            }
        }
    }
    while (1) {
        delay(5);
        if (numOfProcesses == 2) {
            printRoundNumber();
            newLine();
            sendToReferee();
            updateScores(cnt1, cnt2);
            isFinished();
            numOfProcesses = 0;
            makeAnotherGuess();
        }
    }
    return 0;
}

void signalHandler(int signalNumber) {
    numOfProcesses += (signalNumber == SIGINT);
    numOfProcesses += (signalNumber == SIGQUIT);
}

void makeAnotherGuess() {
    /* Signaling players to do another match. */
    for (int i = 0; i < NUMBER_OF_CHILDREN - 1; i++) {
        kill(childID[i], SIGUSR1);
    }
}

void updateScores(int cnt1, int cnt2) {
    totalScore1 += cnt1;
    totalScore2 += cnt2;
}

void isFinished() {
    /* All the extra details below are there for formatting nothing else. */
    char str[MAX_SZ];
    char str2[MAX_SZ];
    int tempCnt1 = countDigits(cnt1);
    int tempCnt2 = countDigits(cnt2);
    if (tempCnt1 > 1 && tempCnt1 > 1) {
        sprintf(str, "%s", "========");
        sprintf(str2, "%s", "========");
    } else if (tempCnt1 > 1 || tempCnt1 > 1) {
        sprintf(str, "%s", "=========");
        sprintf(str2, "%s", "========");
    } else {
        sprintf(str, "%s", "=========");
        sprintf(str2, "%s", "=========");
    }
    if (totalScore1 >= 50 || totalScore2 >= 50) {
        printf(
            " \033[37m[%s \033[32m[Player1: %d]\033[37m vs \033[36m[Player2: "
            "%d]\033[37m "
            "%s]",
            str, cnt1, cnt2, str2);
        newLine();

        int num = 0;
        if (totalScore1 < 50) {
            num = 2;
        } else if (totalScore2 < 50) {
            num = 1;
        }

        printf("\n \033[37mThe game ended with \033[31m %d rounds\033[37m",
               numOfRounds);
        newLine();
        if (num == 0) {
            printf(
                " \033[37mBoth Players are winning with total score "
                "\033[32m[Player1: "
                "%d]\033[37m vs \033[36m[Player2: %d]\033[37m",
                totalScore1, totalScore2);
            newLine();
        } else {
            printf(
                " The winner is Player%d with total score \033[32m[Player1: "
                "%d]\033[37m vs \033[36m[Player2: %d]\033[37m",
                num, totalScore1, totalScore2);
            newLine();
        }
        newLine();
        killWholeProgram();
    } else {
        printf(
            " [%s \033[32m[Player1: %d]\033[37m vs \033[36m[Player2: "
            "%d]\033[37m "
            "%s]",
            str, cnt1, cnt2, str2);

        newLine();
        printf(
            "\n The total scores are \033[32m[Player1: %d]\033[37m vs "
            "\033[36m[Player2: %d]\033[37m",
            totalScore1, totalScore2);
        newLine();
    }
}

void sendToReferee() {
    close(fd1[1]);
    close(fd2[0]);

    char message[MAX_SZ];
    sprintf(message, "%s-%s", "child1.txt", "child2.txt");

    if (write(fd2[1], message, STR_LEN) == -1) {
        perror("Error in wrting process");
        exit(4);
    }

    char receivedMsg[MAX_SZ];
    if (read(fd1[0], receivedMsg, STR_LEN) == -1) {
        perror("Error in reading process");
        exit(4);
    }

    char* token = strtok(receivedMsg, "-");
    char scores[2][MAX_SZ];
    int cnt = 0;

    while (token != NULL) {
        sprintf(scores[cnt++], "%s", token);
        token = strtok(NULL, "-");
    }

    cnt1 = atoi(scores[0]);
    cnt2 = atoi(scores[1]);
}

void refereeWork() {
    /* Some delay for R for more safe. */
    delay(1);

    close(fd1[0]);
    close(fd2[1]);

    /* Reading the father message. */
    char receivedMsg[MAX_SZ];
    if (read(fd2[0], receivedMsg, STR_LEN) == -1) {
        perror("Error in reading process");
        exit(4);
    }

    /* The files will be arrived as 'name1.txt-name2.txt' format so it should be
     * tokenized. */
    char* token = strtok(receivedMsg, "-");
    char nameOfFiles[2][MAX_SZ]; /* the name of files will be store here. */
    int cnt = 0;

    while (token != NULL) {
        sprintf(nameOfFiles[cnt++], "%s", token);
        token = strtok(NULL, "-");
    }

    /* Open the files which are created by players in advance. */
    FILE* filePtr1 = fopen(nameOfFiles[0], "r");
    FILE* filePtr2 = fopen(nameOfFiles[1], "r");

    if (filePtr1 == NULL || filePtr2 == NULL) {
        perror("Error in opening file");
        exit(4);
    }

    /* They are global variable which used in different places so they should be
     * cleared every time. */
    cnt1 = 0;
    cnt2 = 0;

    int arr1[ARRAY_SIZE];
    int arr2[ARRAY_SIZE];

    /* Reading value by value and calculating them. */
    for (int i = 0; i < ARRAY_SIZE; i++) {
        fscanf(filePtr1, "%d", &arr1[i]);
        fscanf(filePtr2, "%d", &arr2[i]);
        cnt1 += (arr1[i] > arr2[i]);
        cnt2 += (arr1[i] < arr2[i]);
    }

    /* Printing the random arrays on terminal. */
    printArray(arr1, ARRAY_SIZE, 1);
    printArray(arr2, ARRAY_SIZE, 2);

    /* Close the open files in os. */
    fclose(filePtr1);
    fclose(filePtr2);

    /* The R must remove the files after calculateing scores. */
    if (remove("child1.txt") != 0 || remove("child2.txt") != 0) {
        perror("The file is not deleted.");
        exit(6);
    }

    /* Make the result in score1-score2 format. */
    char result[MAX_SZ];
    sprintf(result, "%d-%d", cnt1, cnt2);

    /* Writing to father. */
    if (write(fd1[1], result, STR_LEN) == -1) {
        perror("Error in wrting process");
        exit(5);
    }
}

void killWholeProgram() {
    for (int i = 0; i < NUMBER_OF_CHILDREN; i++) {
        kill(childID[i], SIGKILL);
    }
    closeDesc();
    exit(0);
}

void newLine() {
    printf("\n");
    fflush(stdout);
}

void delay(int n) {
    for (int i = 0; i < n; i++) {
        sleep(1);
    }
}

void closeDesc() {
    close(fd1[0]);
    close(fd1[1]);
    close(fd2[0]);
    close(fd2[1]);
}

void printArray(int* arr, int n, int p) {
    printf(" [P%d ===> ", p);
    fflush(stdout);
    for (int i = 0; i < n; i++) {
        int numOfDigits = countDigits(arr[i]);
        if (numOfDigits == 1) {
            printf("%d   ", arr[i]);
        } else if (numOfDigits == 2) {
            printf("%d  ", arr[i]);
        } else {
            printf("%d ", arr[i]);
        }
        fflush(stdout);
    }
    printf("]");
    newLine();
}

void printRoundNumber() {
    numOfRounds += 1;
    if (countDigits(numOfRounds) > 1) {
        printf(
            " \033[37m[====================\033[31m[Round#%d]\033[37m=========="
            "========]",
            numOfRounds);
    } else {
        printf(
            " \033[37m[====================\033[31m[Round#%d]\033[37m=========="
            "=========]",
            numOfRounds);
    }
}

int countDigits(int n) {
    /* I assumed that 0 will take one digit to be printed out in terminal. */
    if (n == 0) {
        return 1;
    }

    int cnt = 0;
    while (n != 0) {
        cnt++;
        n /= 10;
    }
    return cnt;
}
