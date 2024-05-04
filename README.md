# pintos_2

### 교수님이 주신 pintos1_ans.tar파일 사용

---
#### **1. Argument passsing**

___Current pintos___ <br>

- No distinction between programs and arguments. Ex) If you enter “bin/ls –l foo bar“ on the command line, Pintos recognizes "bin/ls -l foo bar" as a single program name
- The function "process_execute()" loads and executes process into memory so that the user can execute the command entered
- Although the program is stored as in "file_name", "process_execute()" currently does not provide argument passing for the new process <br><br>

___Main Goal___ <br>

- develop a feature to parse the command line string, store it on the stack, and pass arguments to the program.
- Instead of simply passsing the program file name as an argument in "process_executer()", you should modify it to parse words each time a space is encountered.
- In this case, the first word is the program name, and the second and third words are the first and second arguments. <br><br>

___Need to add/modify___ <br>
- Modify "tid_t process_execute(const char *file_name)" (userprog/process.c)
- Modify "start_process(void *file_name_)" (userprog/process.c)
- Add "static void argument stack(const char*argv[], int argc, void **esp)" (userprog/process.c)
