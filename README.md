# pintos_2

### 교수님이 주신 pintos1_ans.tar파일 사용

---
#### 1. Argument passsing

___Current Pintos___

- No distinction between programs and arguments. Ex) If you enter “bin/ls –l foo bar“ on the command line, Pintos recognizes "bin/ls -l foo bar" as a single program name
- The function "process_execute()" loads and executes process into memory so that the user can execute the command entered
- Although the program is stored as in "file_name", "process_execute()" currently does not provide argument passing for the new process <br><br>

___Main Goal___

- develop a feature to parse the command line string, store it on the stack, and pass arguments to the program.
- Instead of simply passsing the program file name as an argument in "process_executer()", you should modify it to parse words each time a space is encountered.
- In this case, the first word is the program name, and the second and third words are the first and second arguments. <br><br>

___Need to Add/Modify___

- Modify "tid_t process_execute(const char *file_name)" (userprog/process.c)
- Modify "start_process(void *file_name_)" (userprog/process.c)
- Add "static void argument stack(const char*argv[], int argc, void **esp)" (userprog/process.c) <br><br><br>

#### 2. System call

___Main Goal___

- Current pintos doesn't have implemented system call handlers, so system calls are not processed.
- We should..
    - implement system call handler and system calls
    - add system calls to provide services to users in system call handler; 'halt', 'exit', 'exec', 'wait' <br><br>

___Sytem Call___

- Programming interface for services provided by the operating system
- Allow user mode programs to use kernel features
- System calls run on kernel mode and return to user mode
- Key point of system call is that priority of execution mode is raised to the special mode as hardware interrupts are generated to call system call <br><br>

___Process Hierarchy___

- Augment the existing process with the process hierarchy
- When implementing system calls, it's important to consider the relationship between parent and child processes <br><br>

___System Call Handler___

- Call the system call from the system call handler using the system call number. <br><br>

___Need to Add/Modify___

**threads/thread.**

- Modify thread structure (threads/thread.h) <br>

**userprog/syscall.**

- Modify "void syscall_init(void)" (userprog/syscall.c)
- Modify "static void syscall_handler(struct intr_frame *f)" (userprog/syscall.c)
- Add "bool is_valid_ptr (const void *usr_ptr)" (userprog/syscall.c)
- Add "int wait (pid_t pid)" (userprog/syscall.c)
- Add "void exit (int status)" (userprog/syscall.c)
- Add "pid_t exec (const char *cmd_line)" (userprog/syscall.c)
- Add "void halt(void)" (userprog/syscall.c)

**userprog/process.**

- Modify "tid_t process_execute(const char *file_name)" (userprog/process.c)
- Modify "int process_wait(tid_t child_tid)" (userprog/process.c)
