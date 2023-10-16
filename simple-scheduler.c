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

typedef struct schedInfo
{
    int pid;
    char name[1000];
    int execTime;
    int waitTime;
} schedInfo;

typedef struct shmbuf
{
    int isRunning;
    sem_t sem_lock;
    int ncpu;
    int tslice;
    int pid_cnt;
    schedInfo pids[BUF_SIZE];
    int completed_cnt;
    schedInfo completed[BUF_SIZE];
} shmbuf;

int shm_fd;
struct shmbuf *shmPointer;

void shm_init(char *shmpath)
{

    // Opening the shared memory object
    int shm_fd = shm_open(shmpath, O_RDWR, 0);
    if (shm_fd == -1)
    {
        printf("Couldn't open the shared memory\n");
        return;
    }

    // Map the shared memory address space
    shmPointer = mmap(NULL, sizeof(*shmPointer), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shmPointer == MAP_FAILED)
    {
        printf("Mapping shared memory failed\n");
        exit(1);
    }
}

// dequeues stuff from the proccess table
schedInfo pid_dequeue()
{
    schedInfo dequeued = shmPointer->pids[0];
    for (int i = 1; i < shmPointer->pid_cnt; i++)
    {
        shmPointer->pids[i - 1] = shmPointer->pids[i];
    }
    return dequeued;
}

// enqueues to process table
void pid_enqueue(schedInfo pid)
{
    shmPointer->pids[shmPointer->pid_cnt] = pid;
    shmPointer->pid_cnt++;
}

// enqueues to the completed table
void completed_enqueue(schedInfo pid)
{
    shmPointer->completed[shmPointer->completed_cnt] = pid;
    shmPointer->pid_cnt++;
}

int main()
{
    char *path = "Queue";
    shm_init(path);
    
    // infinitely schedules until the shell is exited
    while (shmPointer->isRunning)
    {
        // checks the current time for if the program exits before tslice
        clock_t start_time;
        clock_t end_time;
        start_time = clock();

        // checks how many proccess are running
        int numRunning;
        if (shmPointer->ncpu < shmPointer->pid_cnt)
        {
            numRunning = shmPointer->ncpu;
        }
        else
        {
            numRunning = shmPointer->pid_cnt;
        }

        // creates an array of proccesses that are running
        schedInfo pid_running[numRunning];
        
        // locks the table
        sem_wait(&shmPointer->sem_lock);

        for (int i = 0; i < numRunning; i++)
        {
            // dequeues the process and enqueues to pid_running
            pid_running[i] = pid_dequeue();
            // continues the process
            kill(pid_running[i].pid, SIGCONT);
        }

        // unlocks the table
        sem_post(&shmPointer->sem_lock);

        // waits for tslice
        usleep(shmPointer->tslice); 

        // locks the table
        sem_wait(&shmPointer->sem_lock);

        // adds the wait time to all the waiting processes
        for (int i = 0; i < shmPointer->pid_cnt; i++)
        {
            pid_running[i].waitTime += shmPointer->tslice;
        }

        for (int i = 0; i < numRunning; i++)
        {
            int status;
            int pid;

            // stops the process
            kill(pid_running[i].pid, SIGSTOP);
            // safely removes process that are completed
            pid = waitpid(pid_running[i].pid, &status, WNOHANG);

            // checks if process is exited
            if (WIFEXITED(status))
            {
                // time is calculated
                end_time = clock();
                double duration = (double)(end_time - start_time) / CLOCKS_PER_SEC;
                duration *= 1000;
                pid_running[i].execTime += (int) duration;
                // enqueued to completed processes
                completed_enqueue(pid_running[i]);
            }
            else if (WIFSTOPPED(status)) // checks if process is stopped
            {
                // execution time is incrmented
                pid_running[i].execTime += shmPointer->tslice;
                // enqueued to running processes
                pid_enqueue(pid_running[i]);
            }
        }
        // unlock the table
        sem_post(&shmPointer->sem_lock);

    }

    // unlinks and munmap
    shm_unlink(path);
    munmap(shmPointer, sizeof(*shmPointer));
    return 0;
}
