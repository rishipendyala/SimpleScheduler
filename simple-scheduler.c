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

typedef struct shmbuf
{
    sem_t sem_data_available;
    sem_t sem_data_processed;
    int ncpu;
    int tslice;
    int pid_cnt;
    int pids[BUF_SIZE];
} shmbuf;


/*

void makeRunning(LinkedList *ready, LinkedList *running)
{
    execInfo p = removeNode(ready);
    strcpy(p->state, "RUNNING");
    append(running, p);
}

void makeReady(LinkedList *ready, LinkedList *running, execInfo *p)
{
    execInfo p = removeNode(running);
    strcpy(p->state, "READY");
    append(ready, p);
}

execInfo removeNode(LinkedList *list)
{
    execInfo *current = list->head;
    execInfo *prev = NULL;

    while (current->next != NULL)
    {
        prev = current;
        current = current->next;
    }

    if (prev != NULL)
    {
        prev->next = NULL;
    }
    else
    {
        list->head = NULL;
    }

    return current;
    // free(current); // Free the removed node
}

// SIGNALS FOR STARTING, PAUSING, TERMINATING, etc.
void pauseProcess(int pid)
{
    int paused = kill(pid, SIGSTOP);
    if (paused == 0)
    {
        printf("Process stopped successfully!! \n");
    }
    else
    {
        printf("Error stopping process\n");
    }
}

void resumeProcess(int pid)
{
    int cont_result = kill(pid, SIGCONT);
    if (cont_result == 0)
    {
        printf("Process resumed successfully.\n");
    }
    else
    {
        printf("Error resuming process\n");
    }
}

void schedulingAlrgorithm(time_t TSLICE)
{
    // ROUND ROBIN
}

// the function below is pseudocode, not C code.
// taken from lecture slides
void scheduler()
{
    while (true)
    {
        lock(processTable);
        forEach(execInfo p
                : scheduling_algorithm(processTable))
        {
            if (p->state != READY)
            {
                continue;
            }
            p->state = RUNNING;
            unlock(processTable);
            swtch(scheduler_process, p);
            // p is done for now..
            lock(processTable);
        }
        unlock(processTable);
    }
}

// Inspired from 
// https://www.geeksforgeeks.org/program-for-round-robin-scheduling-for-the-same-arrival-time/

void calculateWaitingTime(int processes[], int n, int bt[], int wt[], int quantum) {
    int remainingTime[n];
    for (int i = 0; i < n; i++) {
        remainingTime[i] = bt[i];
    }

    int currentTime = 0;

    while (true) {
        bool allDone = true;

        for (int i = 0; i < n; i++) {
            if (remainingTime[i] > 0) {
                allDone = false;

                if (remainingTime[i] > quantum) {
                    currentTime += quantum;
                    remainingTime[i] -= quantum;
                } else {
                    currentTime += remainingTime[i];
                    int executionTime = currentTime;
                    wt[i] = executionTime - bt[i];
                    remainingTime[i] = 0;
                   
                }
            }
        }

        if (allDone) {
            break;
        }
    }
}

void calculateExecutionTime(int processes[], int n, int bt[], int wt[], int quantum) {
    int remainingTime[n];
    for (int i = 0; i < n; i++) {
        remainingTime[i] = bt[i];
    }

    int currentTime = 0;

    while (true) {
        bool allDone = true;

        for (int i = 0; i < n; i++) {
            if (remainingTime[i] > 0) {
                allDone = false;

                if (remainingTime[i] > quantum) {
                    currentTime += quantum;
                    remainingTime[i] -= quantum;
                } else {
                    currentTime += remainingTime[i];
                    int executionTime = currentTime;
                    wt[i] = executionTime - bt[i];
                    remainingTime[i] = 0;
                    
                }
            }
        }

        if (allDone) {
            break;
        }
    }
}
*/

int shm_fd;
struct shmbuf *shmPointer;

void shm_init(char *shmpath)
{   

    // Opening the shared memory object
    int shm_fd = shm_open(shmpath, O_RDWR, 0);
    if (shm_fd == -1) {
        printf("Couldn't open the shared memory\n");
        return;
    }

    // Map the shared memory address space
    shmPointer = mmap(NULL, sizeof(*shmPointer), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if (shmPointer == MAP_FAILED) {
        printf("Mapping shared memory failed\n");
        return;
    }

    // Some initialisation...

    // Update the semaphore (sem_post)
    if (sem_post(&shmPointer->sem_data_available) == -1) {
        printf("Couldn't update the semaphore\n");
        return;
    }

    // Wait for the shell to access memory
    if (sem_wait(&shmPointer->sem_data_processed) == -1) {
        printf("Waiting for the shell failed\n");
        return;
    }
}

int pid_dequeue() 
{
    int dequeued = shmPointer->pids[0];
    for (int i = 1; i < shmPointer->pid_cnt; i++)
    {
        shmPointer->pids[i - 1] = shmPointer->pids[i];
    }
    return dequeued;
}

int pid_enqueue(int pid)
{
    shmPointer->pids[shmPointer->pid_cnt] = pid;
    shmPointer->pid_cnt++;
}

int main()
{   
    char* path = "Queue";
    shm_init(path);

    while (true) 
    {
        int numRunning;
        if(shmPointer->ncpu < shmPointer->pid_cnt)
        {
            numRunning = shmPointer->ncpu;
        } else 
        {
            numRunning = shmPointer->pid_cnt;
        }
        int pid_running[numRunning];


        // figure out lock(pids)
        for (int i = 0; i < numRunning; i++)
        {
            pid_running[i] = pid_dequeue();
            kill(pid_running[i], SIGCONT);
        }


        // figure out unlock(pids)
        usleep(shmPointer->tslice);


        // figure out lock(pids)
        for (int i = 0; i < numRunning; i++)
        {
            if (kill(pid_running[i], SIGSTOP) == 0)
            {
                pid_enqueue(pid_running[i]);
            }
            else
            {
                printf("Process completed with pid %d", pid_running[i]);
            }
        }
        // figure out unlock(pids)
    }

    //LinkedList ready;
    //initializeLinkedList(&ready);

    shm_unlink(path);
    munmap(shmPointer, sizeof(*shmPointer));
    return 0;
}
