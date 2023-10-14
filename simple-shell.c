#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

#define BUF_SIZE 1024

// structure that stores the information of the child process
typedef struct execInfo
{
    int pid;
    time_t timeExecuted;
    float duration;
    int exitStatus;
} execInfo;

// 
typedef struct shmbuf
{
    sem_t sem;
    size_t pid_cnt;
    int pids[BUF_SIZE];
} shmbuf;

// an array for child process info along with a value that tracks the arrays length
int currentInfo = 0;
execInfo infoArray[1000];

// an array with history of commands along with a value that tracks the arrays length
int currentHistory = 0;
char historyArray[1000][1000];

// 
int shm_fd;

void shm_init(char *shmpath)
{
    
}

// prints the process information in the infoArray
void printProcessInfo()
{
    printf("\n\nCOMMANDS RAN: %d\n", currentInfo);
    for (int i = 0; i < currentInfo; i++)
    {
        printf("\n");
        printf("PID: %d\n", infoArray[i].pid);
        struct tm *info = localtime(&infoArray[i].timeExecuted);
        printf("TIME STARTED: %s", asctime(info));
        printf("DURATION: %f\n", infoArray[i].duration);
        printf("EXIT STATUS: %d\n", infoArray[i].exitStatus);
        printf("\n");
    }
}

// prints the history of commands executed
void history()
{
    for (int i = 0; i < currentHistory; i++)
    {
        printf("%s", historyArray[i]);
    }
}

// creates child proccesses and runs them
void create_process_and_run(char **args, int argscount)
{
    // creates a child process
    int status = fork();
    if (status < 0)
    {
        printf("Something bad happened\n");
        exit(1);
    }
    if (status == 0)
    {
        // history command can be executed
        if (strcmp(args[0], "history") == 0)
        {
            history();
            exit(0);
        }
        else if (strcmp(args[0], "submit"))
        {
            
        }
        // any valid exec command can be executed
        else
        {
            int executionStatus = execvp(args[0], args);
            if (executionStatus == -1)
            {
                printf("Invalid command\n");
                exit(1);
            }
        }
    }
    else
    {
        // variables required for processInfo
        execInfo processInfo;
        clock_t start_time;
        clock_t end_time;
        time_t timeExecuted;

        // waiting and information that is required to calculate some execInfo
        time(&timeExecuted);
        start_time = clock();
        int ret;
        int pid = wait(&ret);
        end_time = clock();
        double duration = (double)(end_time - start_time) / CLOCKS_PER_SEC;

        // checks if the command is safely executed
        if (!WIFEXITED(ret))
        {
            printf("Abnormal termination of %d\n", pid);
            return;
        }

        // saves all the execInfo
        processInfo.pid = pid;
        processInfo.timeExecuted = timeExecuted;
        processInfo.duration = duration;
        processInfo.exitStatus = WEXITSTATUS(ret);

        // adds the processInfo to infoArray and updates its size
        infoArray[currentInfo] = processInfo;
        currentInfo++;
    }
}

void launch(char *userInput)
{
    // saving history
    if (currentHistory >= 1000)
    {
        // if history size is depleted it starts from the first history entry
        printf("Maximum history size reached! Resetting from 0.\n");
        currentHistory = 0;
    }
    // copies the userInput to historyArray
    strcpy(historyArray[currentHistory], userInput);
    currentHistory++;

    // creates arguments by using tokenizer so that it can be passed into the execvp function
    char *args[1000] = {NULL};
    char *token = strtok(userInput, " \n\t\r\a\"");
    int count = 0;
    // loops through the input using the tokenizer until it is completely tokenized
    while (token != NULL && count < 999)
    {
        if (strlen(token) > 0)
        {
            args[count] = token;
            count++;
        }
        token = strtok(NULL, " \n\t\r\a\"");
    }

    // checks if args exists
    if (args[0] != NULL)
    {
        // executes the user input
        create_process_and_run(args, count);
    }
    else
    {
        // redudant print statment (just letting you know this wasn't an oversight)
        printf("\n");
    }
}

// Ctrl+C input is redirected to this function
void exit_program()
{
    printProcessInfo();
    exit(0);
}

int main()
{
    // reads the ctrl+C input and redirects it to exit_program
    signal(SIGINT, exit_program);

    // intialized userInput to be read
    char userInput[1000];

    do
    {
        // reads the input with a nice terminal interface
        printf("> ");
        fgets(userInput, sizeof(userInput), stdin);

        // runs every command user has inputted
        launch(userInput);
        // previously i had added a new line but it doesn't make sense because bash never does it either

    } while (true);
    return 0;
}
