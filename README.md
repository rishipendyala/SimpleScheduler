# Linux Shell with Process Scheduling

## Overview

This project implements a custom Linux shell with basic command execution and process scheduling capabilities. It uses shared memory and semaphores to manage and schedule processes, and it maintains a history of executed commands.

## Features

- **Basic Shell Functionality**: Execute standard shell commands.
- **Process Scheduling**: Submit and schedule processes with different priorities.
- **Shared Memory**: Utilize shared memory to communicate between parent and child processes.
- **Semaphores**: Ensure proper synchronization using semaphores.
- **History Tracking**: Keep a history of executed commands.
- **Signal Handling**: Gracefully handle the termination signal (Ctrl+C) to print process information before exiting.

## Structures

### `schedInfo`

Stores information about a scheduled process.

```c
typedef struct schedInfo {
    int pid;
    char name[1000];
    int execTime;
    int waitTime;
    int priority;
} schedInfo;
```

### `execInfo`

Stores information about an executed child process.

```c
typedef struct execInfo {
    int pid;
    time_t timeExecuted;
    float duration;
    int exitStatus;
} execInfo;
```

### `shmbuf`

Shared memory buffer structure.

```c
typedef struct shmbuf {
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
```

## Functions

### `void shm_init(char *shmpath, int ncpu, int tslice)`

Initializes shared memory and semaphores.

### `void printProcessInfo()`

Prints information about all executed processes.

### `void history()`

Prints the history of executed commands.

### `void create_process_and_run(char **args, int argscount)`

Creates and runs child processes based on the provided arguments.

### `void launch(char *userInput)`

Parses user input and executes the appropriate command.

### `void exit_program()`

Handles the termination signal (Ctrl+C) and prints process information before exiting.

## Usage

### Compilation

Compile the code using `gcc`:

```bash
gcc -o custom_shell custom_shell.c -pthread -lrt
```

### Execution

Run the compiled program:

```bash
./custom_shell
```

### Commands

- **Basic Commands**: Any standard shell command can be executed.
- **Submit Commands**: Use the `submit` keyword to schedule a process. For example, `submit ls -l`.
- **History**: The `history` command prints the history of executed commands.

### Sample Interaction

```
Enter number of CPUs: 2
Enter tslice in milliseconds: 100

Waiting for scheduler
Scheduler hooked

> ls
<output of ls command>

> submit ls -l
<scheduled output of ls -l command>

> history
ls
submit ls -l
```

## Notes

- Ensure you have appropriate permissions to create shared memory objects.
- The `submit` keyword is used to schedule commands with a specified priority.
- The program gracefully handles Ctrl+C, printing process information before exiting.

## Dependencies

- **pthread library**: Used for semaphores.
- **librt**: Used for shared memory.

```bash
gcc -o custom_shell custom_shell.c -pthread -lrt
```
