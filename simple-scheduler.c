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
    int isRunning;
    sem_t sem_lock;
    int ncpu;
    int tslice;
    int pid_cnt;
    int pids[BUF_SIZE];
} shmbuf;

/*
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
*/

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
    char *path = "Queue";
    shm_init(path);

    printf("%d", shmPointer->isRunning);
    
    while (shmPointer->isRunning)
    {
        int numRunning;
        if (shmPointer->ncpu < shmPointer->pid_cnt)
        {
            numRunning = shmPointer->ncpu;
        }
        else
        {
            numRunning = shmPointer->pid_cnt;
        }
        int pid_running[numRunning];
        for (int i = 0; i < numRunning; i++)
        {
            pid_running[i] = 0;
        }
        
        sem_wait(&shmPointer->sem_lock);

        for (int i = 0; i < numRunning; i++)
        {
            pid_running[i] = pid_dequeue();
            kill(pid_running[i], SIGCONT);
        }

        sem_post(&shmPointer->sem_lock);

        usleep(shmPointer->tslice); 

        
        sem_wait(&shmPointer->sem_lock);

        for (int i = 0; i < numRunning; i++)
        {
            int status;
            int pid;

            kill(pid_running[i], SIGSTOP);
            pid = waitpid(pid_running[i], &status, WNOHANG);
            if (WIFEXITED(status))
            {
                printf("Process completed with pid %d with status %d\n", pid_running[i], status);
            }
            else if (WIFSTOPPED(status))
            {
                pid_enqueue(pid_running[i]);
            }
        }

        sem_post(&shmPointer->sem_lock);

    }

    shm_unlink(path);
    munmap(shmPointer, sizeof(*shmPointer));
    return 0;
}
