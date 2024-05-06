#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
// pt 2-2 추가 include
#include "userprog/pagedir.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"

typedef int pid_t;

static void syscall_handler (struct intr_frame *);
bool is_valid_ptr (const void *usr_ptr);
int wait (pid_t pid);
void exit (int status);
pid_t exec (const char *cmd_line);
void halt(void);

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
  if (!is_valid_ptr(f->esp)) exit (-1);

  int *p = f->esp;


  // pt 2-2 halt, exit, exec, wait 에 대한 systemcall handler 
  switch(*p)
  {
    case SYS_HALT :
      halt();
      break;
    
    case SYS_EXIT :
      exit(*(p + 1));
      break;
    
    case SYS_EXEC :
      f->eax = exec((char *) *(p + 1));
      break;
    
    case SYS_WAIT :
      f->eax = wait(*(p + 1));
      break;
  }

  
  printf ("system call!\n");
  thread_exit ();
}

// pt 2-2 ptr이 vaild한지 확인
bool is_valid_ptr (const void *usr_ptr)
{
  struct thread *cur = thread_current();
  
  // pt 2-2 NULL 인지 확인
  if (usr_ptr == NULL) return false;

  // pt 2-2 user area인지, 범위 내에 있는지 확인
  return is_user_vaddr (usr_ptr) && pagedir_get_page (cur->pagedir, usr_ptr);
}

// pt 2-2 handler methods

int wait (pid_t pid)
{
  process_wait (pid);
}

void exit (int status)
{
  return;
}

pid_t exec (const char *cmd_line)
{
  tid_t tid;
  struct thread *cur = thread_current();

  if(!is_valid_ptr(cmd_line)) exit(-1);

  cur->child_load_status = 0;
  tid = process_execute(cmd_line);
  lock_acquire(&cur->lock_child);

  while (cur->child_load_status == 0)
    cond_wait(&cur->cond_child, cur->lock_child);

  if (cur->child_load_status == -1)
    tid = -1;

  lock_release(&cur->lock_child);

  return tid;
}

void halt(void)
{
  shutdown_power_off();
}
