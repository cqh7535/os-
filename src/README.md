# pintos-user-program
## Overview
This project is to finish three tasks in the user program realization part of pintos. To finish these task, firstly, I read the related resources about pintos and try my best to understand the code. And then, modify the code according to the task requirements. This project aims to implement the necessary features for user programs (the test programs) to request kernel functionality. We need to finish three tests: argument passing, process control syscalls, file operation syscalls.

## Methodology
### Task1 - Argument Passing
#### Main Ideas
Currently, pintos does not support command-line arguments. We must implement argument passing. To understand the process of argument passing, we need three steps:
- Parsing the command line
- Put the argument into stack
- Check the safe memory access
#### Implementation
- Add a function string partition method, which minus the stack pointer 4 bytes once and copy 4 bytes from memory ptr points at to esp points at (the stack). Modify the function execute_process to cut the long cmd and give the process a real name.
- Modify the function setup_stack to count the number of args, parse the args and push the stack to store args and address one by one. Use function strtok_r to cut the command. Notice: after pushing the args, there’s a word-align needed to be paid attention to.

### Task2 - Process Control Syscalls
#### Main Ideas
To finish this task, we must understand how a system call is happened, and how the operation system change from the user space to the kernel space. Therefore, I summary the main steps of a system call happened in the following.
- Pintos uses int 0x30 for system calls
- Pintos has code for dispatching syscalls from user programs. i.e. user processes will push parameters onto the stack and execute int 0x30
- In the kernel, Pintos will handle int 0x30 by calling syscall_handler() in userprog/syscall.c

In task2, I will add support for the following new syscalls: halt, exec, wait, and practice. Each of these syscalls has a corresponding function inside the userlevel library, lib/user/syscall.c.The kernel’s syscall handlers are located in userprog/syscall.c.

- Halt: shutdown the system.
- Exec: start a new program with process_execute().The Pintos exec syscall is similar to calling Linux’s fork syscall and then Linux’s execve syscall in the child process immediately afterward.)
- Wait: wait for a specific child process to exit. This system call is the most complicated one in this project.

#### Implementation

- For process execute function, call sema_down for the current running thread to make it synchronized for the execution. If the process is executed successfully, return the child process’s tid. If the process isn’t executed successfully, then return TID_ERROR. 
- SYS_HALT: Just add function shutdown_power_off().
- For process wait function, firstly, if the arg is TID_ERROR, return -1 as an error message. For every process, it will always setup a semaphore for his parent process to wait. The parent process will wait for the child process’s termination and then get the semaphore. Because the system will free all information of a dead process. We then use the struct heritage to keep these information.
- SYS_EXIT: Firstly, check if the argument it passed (the exit code) is valid. If it’s valid, pass it to current thread’s exit_code and call thread_exit(), which is modified also and will be described later.
- SYS_EXEC: Firstly, check if the argument it passed (the file name pointer) is valid and check if the content that the pointer points at is valid, too. If it’s valid, strtok_r the file name and pass it to the function process_execute().
- SYS_WAIT: Firstly, check if the argument it passed (the pid) is valid. Then call the wait function. 

### Task3 – File Operation Syscalls

#### Main Ideas
In addition to the process control syscalls, you will also need to implement these file operation syscalls: create, remove, open, filesize, read, write, seek, tell, and close. The are some things to notice:
- While a user process is running, you must ensure that nobody can modify its executable on disk. The tests ensure that you deny writes to currentrunning program files. The functions file_deny_write() and file_allow_write() can assist the function.
- We must make sure that your file operation syscalls do not call multiple filesystem functions concurrently, The Pintos filesystem is not thread-safe. Every file operation (like create, open, etc.) needs to call function. lock_acquire and lock_release to keep synchronization for file operations.

#### Implementation
- Use a global lock to ensure the file syscalls is thread safe. When every syscall is called, it must acquire lock and after then, release the lock.
- Add a new struct file_obj to keep more information of a single file, whose variable has struct file *ptr to keep the address of a file, int fd to keep the file descriptor of a file, struct list_elem elem to be pushed into a thread’s files list.
- Add new member variables to struct thread: int fd_others to keep the file descriptor larger than STDIN_FILENO and STDOUT_FILENO, list files to keep a thread’s opened files, file *self to keep its own file info.
- For read system call, Firstly, if the file descriptor is STDIN_FILENO, the function will store input to the buffer and then put them to the console. If the file descriptor is larger than or equal to 2, then we traverse current thread’s file list and call file_read() to read several size of contents.
- For READ sysyem call. Firstly, if the file descriptor is STDOUT_FILENO, the function will put content in buffer to the console. If the file descriptor is larger than or equal to 2, then we traverse current thread’s file list and call file_write() to write several size of contents to the file.
- And don not forget to close the file, and then release the resoueces.


## Experiment Verification
<img src="https://github.com/lengyyy/pintos-user-program/blob/master/experiment/Screen%20Shot%202018-12-31%20at%2012.53.16%20AM.png" width="450">

