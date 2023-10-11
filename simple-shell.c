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

int countPipes(char **args, int argscount)
{
    int count = 0;

    for (int i = 0; i < argscount; i++)
    {
        if (strcmp(args[i], "|") == 0)
        {
            count++;
        }
    }
    return count;
}

void history()
{
    for (int i = 0; i < currentHistory; i++)
    {
        printf("%s\n", historyArray[i]);
    }
}

int create_process_and_run(char **args, int n_pipes, int argscount)
{
    int fd[n_pipes][2];
    for (int i = 0; i < n_pipes; i++)
    {
        if (pipe(fd[i]) < 0)
        {
            printf("Something bad happened\n");
            exit(1);
        }
    }

    int i = 0;

    for (int pipes = 0; pipes < n_pipes + 1; pipes++)
    {
        char *new_args[1000];
        int j = 0;
        while (i < argscount)
        {
            if (strcmp(args[i], "|") == 0)
            {
                new_args[j] = NULL;
                break;
            }
            new_args[j] = args[i];
            i++;
            j++;
        }
        i++;
        int status = fork();
        if (status < 0)
        {
            printf("Something bad happened\n");
            exit(1);
        }
        if (status == 0)
        {
            if (pipes != 0)
            {
                // Redirect input from the previous pipe
                if (dup2(fd[pipes - 1][0], 0) < 0)
                {
                    printf("Duplication failed\n");
                    exit(1);
                }
            }

            if (pipes != n_pipes)
            {
                // Redirect output to the next pipe
                if (dup2(fd[pipes][1], 1) < 0)
                {
                    printf("Duplication failed\n");
                    exit(1);
                }
            }
            for (int no_fd = 0; no_fd < n_pipes; no_fd++)
            {
                close_file(fd[no_fd][0]);
                close_file(fd[no_fd][1]);
            }
            if (strcmp(args[0], "history") == 0)
            {
                history();
                exit(0);
            }
            else
            {

                int executionStatus = execvp(new_args[0], new_args);
                if (executionStatus == -1)
                {
                    printf("Invalid command\n");
                    return 0;
                }
            }
        }
        else
        {
            // Parent process close_files file descriptors it doesn't need
            if (pipes != 0)
            {
                close_file(fd[pipes - 1][0]);
                close_file(fd[pipes - 1][1]);
            }
        }
    }

    for (int i = 0; i < n_pipes; i++)
    {
        close_file(fd[i][0]);
        close_file(fd[i][1]);
    }

    for (int i = 0; i < n_pipes + 1; i++)
    {
        execInfo processInfo;
        clock_t start_time;
        clock_t end_time;
        time(&processInfo.timeExecuted);
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
            return 1;
        }
    }
    return 1;
}

void launch(char *userInput)
{
    char *args[1000];
    char *token = strtok(userInput, " ");

    int count = 0;
    while (token != NULL && count < 999)
    {
        args[count] = token;
        token = strtok(NULL, " ");
        count++;
    }
    args[count] = NULL;

    if (args[0] != NULL)
    {
        int pipeCount = countPipes(args, count);
        int isExecuted = create_process_and_run(args, pipeCount, count);
        if (isExecuted)
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
        }
        else
        {
            printf("Command execution failed!\n");
        }
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
    char userInput[1000];
    signal(SIGINT, exit_loop);

    // Code for input of NCPU, TSLICE
    
    int NCPU;
    int TSLICE;

    printf("Enter number of CPUs:\n");
    scanf("%d", &NCPU);
    printf("Enter time slice:\n");
    scanf("%d", &NCPU);

    printf("NCPU = %d and TSLICE = %d",NCPU,TSLICE);

    do
    {
        printf("> ");
        fgets(userInput, sizeof(userInput), stdin);

        // terminate input by replacing newline with null character
        int len = strlen(userInput);
        if (len > 0 && userInput[len - 1] == '\n')
        {
            userInput[len - 1] = '\0';
        }

        launch(userInput);
        printf("\n");

    } while (true);
    return 0;
}