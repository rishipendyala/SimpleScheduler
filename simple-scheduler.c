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
    int priority;
} schedInfo;

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

    sem_post(&shmPointer->sem_check);
    printf("hooked to shell\n");
    
}

// dequeues stuff from the proccess table
schedInfo pid_dequeue()
{
    schedInfo dequeued = shmPointer->pids[0];
    for (int i = 1; i < shmPointer->pid_cnt; i++)
    {
        shmPointer->pids[i - 1] = shmPointer->pids[i];
    }
    shmPointer->pid_cnt--;
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

void priority_sort()
{
    for (int i = 0; i < shmPointer->pid_cnt; i++)
    {
        for (int j = i + 1; j < shmPointer->pid_cnt; j++)
        {
            if (shmPointer->pids[i].priority<shmPointer->pids[j].priority)
            {
                schedInfo temp = shmPointer->pids[j];
                shmPointer->pids[j] = shmPointer->pids[i];
                shmPointer->pids[i] = shmPointer->pids[j];
            }
        }
    }
}

int main()
{
    char *path = "Queue";
    shm_init(path);
    
    // infinitely schedules until the shell is exited
    while (shmPointer->isRunning)
    {
        if (shmPointer->pid_cnt == 0)
        {
            continue;
        }
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

        // my heuristic for priority queueing is just sorting
        priority_sort();
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
            shmPointer->pids[i].waitTime += shmPointer->tslice;
        }

        for (int i = 0; i < numRunning; i++)
        {
            if (pid_running[i].pid == 0)
            {
                continue;
            }

            // stops the process
            if (kill(pid_running[i].pid, 0) == -1)
            {
               // time is calculated
                pid_running[i].execTime += shmPointer->tslice;
                // enqueued to completed processes
                completed_enqueue(pid_running[i]);
            }
            else
            {
                // execution time is incrmented
                pid_running[i].execTime += shmPointer->tslice;
                // enqueued to running processes
                kill(pid_running[i].pid, SIGSTOP);
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
