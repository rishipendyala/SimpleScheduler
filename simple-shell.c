#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>

typedef struct execInfo
{
    int pid;
    time_t timeExecuted;
    float duration;
    int exitStatus;
} execInfo;

int currentInfo = 0;
execInfo infoArray[1000];

int currentHistory = 0;
char historyArray[1000][1000];

void close_file(int file_name)
{
    if (close(file_name) == -1)
    {
        printf("Error closing file\n");
    }
}

void dumpProcesses()
{
    printf("\nCOMMANDS RAN: %d\n", currentInfo);
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

void history()
{
    for (int i = 0; i < currentHistory; i++)
    {
        printf("%s", historyArray[i]);
    }
}

void create_process_and_run(char **args, int argscount)
{
    int status = fork();
    if (status < 0)
    {
        printf("Something bad happened\n");
        exit(1);
    }
    if (status == 0)
    {
        if (strcmp(args[0], "history") == 0)
        {
            history();
            exit(0);
        }
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
        execInfo processInfo;
        clock_t start_time;
        clock_t end_time;
        time_t time_result = time(&processInfo.timeExecuted);
        if (time_result == -1)
        {
            printf("Error with reading time\n");
            return;
        }
        start_time = clock();
        int ret;
        int pid = wait(&ret);
        end_time = clock();
        double duration = (double)(end_time - start_time) / CLOCKS_PER_SEC;
        processInfo.pid = pid;
        processInfo.duration = duration;
        processInfo.exitStatus = WEXITSTATUS(ret);
        infoArray[currentInfo] = processInfo;
        currentInfo++;
        if (!WIFEXITED(ret))
        {
            printf("Abnormal termination of %d\n", pid);
            return;
        }
    }
}

void launch(char *userInput)
{
    if (currentHistory < 1000)
    {
        strcpy(historyArray[currentHistory], userInput);
        currentHistory++;
    }
    else
    {
        printf("Maximum history size reached!!\n");
    }
    char *args[1000] = {NULL};
    char *token = strtok(userInput, " \n\t\r\a\"");
    int count = 0;

    while (token != NULL && count < 999)
    {
        if (strlen(token) > 0){
            args[count] = token;
            count++;
        }
        token = strtok(NULL, " \n\t\r\a\"");
    }

    if (args[0] != NULL)
    {
        create_process_and_run(args, count);
    }
    else
    {
        printf("\n");
    }
}

void exit_loop()
{
    dumpProcesses();
    exit(0);
}

int main()
{

    signal(SIGINT, exit_loop);

    // Code for input of NCPU, TSLICE

    // int NCPU;
    // int TSLICE;

    // printf("Enter number of CPUs:\n");
    // scanf("%d", &NCPU);
    // printf("Enter time slice:\n");
    // scanf("%d", &NCPU);

    // printf("NCPU = %d and TSLICE = %d",NCPU,TSLICE);

    char userInput[1000];
    char cwd[1024];
    do
    {
        // for (int i =0; i<1000;  i++){
        //     printf("Argument %d is %s ",i, userInput[i]);
        // }

        printf("%s> ", getcwd(cwd, sizeof(cwd)));
        if (fgets(userInput, sizeof(userInput), stdin) == NULL)
        {
            break; // Handle EOF or fgets error
        }

        // printf("Before string :- %s\n", userInput);

        // terminate input by replacing newline with null character
        // int len = strlen(userInput);
        // if (len > 0 && userInput[len - 1] == '\n')
        // {
        //     // printf("Here\n");
        //     userInput[len - 1] = '\0';
        // }

        // printf("After string :- %s\n", userInput);
        // for (int i = 0; i < strlen(userInput); i++){
        //     printf("char %c\n", userInput[i]);
        // }
        launch(userInput);
        printf("\n");

    } while (true);
    return 0;
}