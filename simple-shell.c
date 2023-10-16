#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <time.h>
#include <fcntl.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>

#define BUF_SIZE 1024

// structure where scheduler proccess information is stored
typedef struct schedInfo
{
    int pid;
    char name[1000];
    int execTime;
    int waitTime;
    int priority;
} schedInfo;

// structure that stores the information of the child process
typedef struct execInfo
{
    int pid;
    time_t timeExecuted;
    float duration;
    int exitStatus;
} execInfo;

// shm buffer
typedef struct shmbuf
{
    int isRunning;
    sem_t sem_lock;
    sem_t sem_check;
    int ncpu;
    int tslice;
    int pid_cnt;
    schedInfo pids[BUF_SIZE];
    int completed_cnt;
    schedInfo completed[BUF_SIZE];
    int pids_completed_cnt;
    int pids_completed[BUF_SIZE];
} shmbuf;

// an array for child process info along with a value that tracks the arrays length
int currentInfo = 0;
execInfo infoArray[1000];

// an array with history of commands along with a value that tracks the arrays length
int currentHistory = 0;
char historyArray[1000][1000];

//
int shm_fd;
struct shmbuf *shmPointer;

void shm_init(char *shmpath, int ncpu, int tslice)
{
    // Following lecture slides, we use shm_open, ftruncate, mmap
    // Create shared memory object
    shm_unlink(shmpath);
    shm_fd = shm_open(shmpath, O_CREAT | O_EXCL | O_RDWR, 0600);
    if (shm_fd == -1)
    {
        printf("Failed to create shared memory object\n");
        return;
    }

    // Allocate memory for the shared object
    if (ftruncate(shm_fd, sizeof(struct shmbuf)) == -1)
    {
        printf("ftruncate failed\n");
        return;
    }

    // Map shared memory
    shmPointer = mmap(NULL, sizeof(*shmPointer), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shmPointer == MAP_FAILED)
    {
        printf("Mapping memory failed\n");
        return;
    }

    // Initialise semaphores
    if (sem_init(&shmPointer->sem_lock, 1, 1) == -1 || sem_init(&shmPointer->sem_check, 1, 0) == -1)
    {
        printf("Semaphore init failed\n");
        return;
    }

    // some info
    shmPointer->ncpu = ncpu;
    shmPointer->tslice = tslice;
    shmPointer->isRunning=1;

    printf("Waiting for scheduler\n");
    sem_wait(&shmPointer->sem_check);
    printf("Scheduler hooked\n");
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

    printf("\n\nSCHEDULED COMMANDS RAN: %d\n", shmPointer->completed_cnt);
    for (int i = 0; i < shmPointer->completed_cnt; i++)
    {
        printf("\n");
        printf("Name: %s\n", shmPointer->completed[i].name);
        printf("PID: %d\n", shmPointer->completed[i].pid);
        printf("Execution Time: %d\n", shmPointer->completed[i].execTime);
        printf("Wait Time: %d\n", shmPointer->completed[i].waitTime);
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
    int isScheduled = 0;
    // if process isScheduled
    if (strcmp(args[0], "submit") == 0)
    {
        isScheduled = 1;
        // remove "submit" from the args as it is not the executable itself
        for (int i = 1; i < 1000; i++)
        {
            args[i - 1] = args[i];
        }
    }
    // creates a child process
    int status = fork();
    if (status < 0)
    {
        printf("Something bad happened\n");
        exit(1);
    }
    if (status == 0)
    {
        if (isScheduled)
        {
            int status2 = fork();
            if (status2 < 0)
            {
                printf("Something bad happened\n");
            }
            else if (status2 == 0)
            {
                // shm is locked
                sem_wait(&shmPointer->sem_lock);
                schedInfo *process = (schedInfo*) malloc(sizeof(schedInfo*));
                // pid of the current process is appended to the process table
                process->pid = getpid();
                process->execTime = 0;
                process->waitTime = 0;
                if (args[1] != NULL)
                {
                    process->priority = atoi(args[1]);
                } else {
                    process->priority = 1;
                }
                shmPointer->pids[shmPointer->pid_cnt] = *process;
                shmPointer->pid_cnt++;
                
                // shm is unlocked
                sem_post(&shmPointer->sem_lock);
                // stops the process
                kill(getpid(), SIGSTOP);
                int executionStatus = execvp(args[0], args);
                if (executionStatus == -1)
                {
                    printf("Invalid command\n");
                    exit(1);
                }
            }
            else
            {
                int ret;
                int pid = wait(&ret);
                if (!WIFEXITED(ret))
                {
                    printf("Abnormal termination of %d", pid);
                }
                sem_wait(&shmPointer->sem_lock);
                shmPointer->pids_completed[shmPointer->pids_completed_cnt] = pid;
                shmPointer->pids_completed_cnt++;
                sem_post(&shmPointer->sem_lock);
            }
            
        }
        // history command can be executed
        else if (strcmp(args[0], "history") == 0)
        {
            history();
            exit(0);
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
        if (!isScheduled)
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
        } else
        {
            
        }
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
    shmPointer->isRunning=0;
    printProcessInfo();
    shm_unlink("Queue");
    munmap(shmPointer, sizeof(*shmPointer));
    exit(0);
}

int main()
{
    int ncpu;
    int tslice;
    printf("Enter number of CPUs: ");
    scanf("%d", &ncpu);
    printf("Enter tslice in milliseconds: ");
    scanf("%d", &tslice);
    tslice *= 1000;

    char *path = "Queue";
    shm_init(path, ncpu, tslice);
    
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
