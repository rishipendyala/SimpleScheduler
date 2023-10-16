# Simple Scheduler

We have implemented a simple scheduler in C from scratch. Once the shell is created the scheduler is run to hook to the shell as a daemon and the shell can schedule commands using "submit".

## Concepts Used
- IPC
- semaphore
- signals
- kill
- grandchild proccesses
- Queue

## Our Approach
1. At the start of both the shell and the scheduler an IPC is created using shm to link the two
2. The scheduler is infinitely looping trying to implement the round robin algorithm
3. The user can enter a command to enter the proccess queue of the scheduler using "submit" as the commandline executable. The process will be immediately SIGSTOP before anything can be run so that it runs in the background
4. The scheduler will SIGCONT the number of processes that the cpus can handle and dequeue them from the processes array and enqueue it to the running array
5. The scheduler will wait for tslice
6. The scheduler will then SIGSTOP all the running proccesses and enqueue them to either the proccess array or completed array depending on what is required
7. The wait time and the execution time is recorded

## Contributions
### Rishi Pendyala
- shm_init
- kill commands
- semaphores (preliminary)

### Vedang Patel
- Raw Code
- Modified Shell Code
- round robin
- semaphores (final)
- creating proccess queues
- proccess info