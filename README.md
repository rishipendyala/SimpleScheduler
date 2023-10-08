# Simple Shell

We have implemented a simple shell in C from scratch. This implementation is based on taking input from the user and executing the valid commands via a child process. 

## Concepts Used 

Tokenising  
Pipeing  
Child proceses - fork, WIFEXITED, wait  
Using Structs  


## Our Approach:

1. Input is obtained from the user through an infinite do-while loop.

2. Within the loop, the `launch` method is called, and the user input is passed as an argument.

3. The `strtok` method is used to tokenize the input, separating each token, and storing them in an array called `args`.

4. The `countPipes` method is then called with `args` as an argument, which counts the number of "|" characters and calculates the number of pipes in the input.

5. After determining the number of pipes, a child process is created and executed to handle the commands. 

6. In the `create_and_run_process` method, pipes are managed to facilitate inter-process communication.

7. The `execvp` command is used to execute the commands within the child process.

8. If the execution of the command succeeds, it is added to the `historyArray` for future reference.

9. The information regarding the process is stored into an array called infoArray which can be accessed later. This facilitates the termination procedure.

10. When CTRL+C is entered as input, the loop terminates. This is implemented by the signal function which calls exit_loop. The information is then dumped and an exit command is executed. 

## Commands that don't work
cd - changing directory inside a child process doesn't cause a change in the parent process. So, when a new child proces is created, it will use the parent's directory.
exit - exiting inside a child process, doesn't exit out of the parent process. 
clear - cannot clear the parent process from within the child process.

## CONTRIBUTIONS

### Rishi Pendyala
- Parsing Input  
- Exec command  
- Shell Termination  
- Error Handling  
- Documentation  

### Vedang Rajendrakumar Patel
- Fork  
- Piplining  
- History  
- Process Dump
- Shell Scripting




