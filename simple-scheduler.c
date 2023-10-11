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
    time_t waitTime;
    char *state;
    execInfo *next;

} execInfo;

typedef LinkedList
{
    execInfo *head;
};
LinkedList;

execInfo processTable[1000];

execInfo *createNode(int pid, time_t timeExecuted, float duration, int exitStatus)
{
    execInfo *newNode = (execInfo *)malloc(sizeof(execInfo));
    if (newNode != NULL)
    {
        newNode->pid = pid;
        newNode->next = NULL;
        strcpy(newNode->state, "READY");
        newNode->timeExecuted = timeExecuted;
        newNode->duration = duration;
        newNode->exitStatus = exitStatus;
    }
    return newNode;
}

void initializeLinkedList(LinkedList *list)
{
    list->head = NULL;
}

void insert(LinkedList *list, int pid, time_t timeExecuted, float duration, int exitStatus)
{
    execInfo *newNode = createNode(pid, timeExecuted, duration, exitStatus);
    if (newNode != NULL)
    {
        if (list->head == NULL)
        {
            list->head = newNode;
        }
        else
        {
            execInfo *current = list->head;
            while (current->next != NULL)
            {
                current = current->next;
            }
            current->next = newNode;
        }
    }
}

void append(LinkedList *list, execInfo *p)
{
    execInfo *t = list->head;
    if (t != NULL)
    {
        while (t->next ! = NULL)
        {
            t = t->next;
        }
        t->next = p;
    }
    else
    {
        t = p;
    }
}

void displayLinkedList(LinkedList *list)
{
    execInfo *current = list->head;
    while (current != NULL)
    {
        printProcess(current);
        current = current->next;
    }
}

void printProcess(execInfo *process)
{
    printf("PID: %d\n", process->pid);
    printf("DURATION: %f\n", process->duration);
    printf("EXIT STATUS: %d\n", process->exitStatus);
    printf("\n");
}

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


int main()
{

    LinkedList ready;
    LinkedList running;

    initializeLinkedList(&ready);
    initializeLinkedList(&running);

    return 0;
}
