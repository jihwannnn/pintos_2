#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

// pt 2-2 추가 include
#include "userprog/pagedir.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "threads/synch.h"
#include "devices/input.h"

// pt 2 pid_t
typedef int pid_t;

static void syscall_handler (struct intr_frame *);

// pt 2-3 struct file_descriptor

struct file_descriptor
{
  int fd_num;                   // uniquely identifying each files of the process
  tid_t owner;                  // id of the thread owning
  struct file *file_struct;     // real file objects
  struct list_elem elem;        // list_elem of files of the specific process
};

struct list open_files; // open list
struct lock fs_lock;    // lock for files
static int fs_id = 0;


// pt 2-2 함수 추가
bool is_valid_ptr (const void *usr_ptr);
int wait (pid_t pid);
void exit (int status);
pid_t exec (const char *cmd_line);
void halt(void);

// pt 2-3 추가
bool create (const char *file_name, unsigned size);
bool remove (const char *file_name);
int open (const char *file_name);
int filesize (int fd);
struct file_descriptor * get_open_file (int fd);
int read (int fd, void *buffer, unsigned size);
int write (int fd, const void *buffer, unsigned size);
void seek (int fd, unsigned position);
unsigned tell (int fd);
void close (int fd);
void close_open_file (int fd);

static int allocate_fd(void);
void close_file_by_owner(tid_t tid);






void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");

  // pt 2-3 init list and lock
  list_init(&open_files);
  lock_init(&fs_lock);
}

static void
syscall_handler (struct intr_frame *f) 
{
  int *p = f->esp;

  if (!is_valid_ptr(p)
  || !is_valid_ptr(p + 1)
  || !is_valid_ptr(p + 2)
  || !is_valid_ptr(p + 3)) exit (-1);

  

  int num = *p;
  // pt 2-2 halt, exit, exec, wait 에 대한 systemcall handler 
  switch(num)
  {
    case SYS_HALT:
      halt();
      break;
    
    case SYS_EXIT:
      exit(*(p + 1));
      break;
    
    case SYS_EXEC:
      f->eax = exec((char *) *(p + 1));
      break;
    
    case SYS_WAIT:
      f->eax = wait(*(p + 1));
      break;
    
    case SYS_CREATE:
      f->eax = create((char *) *(p + 1), *(p + 2));
      break;

    case SYS_REMOVE:
      f->eax = remove((char *) *(p + 1));
      break;

    case SYS_OPEN:
      f->eax = open((char *) *(p + 1));
      break;
        
    case SYS_FILESIZE:
      f->eax = filesize(*(p + 1));
      break;

    case SYS_READ:
      f->eax = read(*(p + 1), (void *) *(p + 2), *(p + 3));
      break;

    case SYS_WRITE:
      f->eax = write(*(p + 1), (void *) *(p + 2), *(p + 3));
      break;

    case SYS_SEEK:
      seek(*(p + 1), *(p + 2));
      break;

    case SYS_TELL:
      f->eax = tell(*(p + 1));
      break;

    case SYS_CLOSE:
      close(*(p + 1));
      break;
    
    default:
      break;
  }

  
  printf ("system call!\n");

  thread_exit ();
}

// pt 2-2 ptr이 vaild한지 확인
bool 
is_valid_ptr (const void *usr_ptr)
{
  struct thread *cur = thread_current();
  
  // pt 2-2 NULL 인지 확인
  if(usr_ptr == NULL) return false;

  // pt 2-2 user area인지, 범위 내에 있는지 확인
  return is_user_vaddr(usr_ptr) && pagedir_get_page(cur->pagedir, usr_ptr);
}

// pt 2-2 handler methods

int 
wait (pid_t pid)
{
  process_wait(pid);
}

void 
exit (int status)
{
  struct thread *cur = thread_current();
  struct child_status *child;
  struct list_elem *e;
  
  struct thread *parent = thread_get_by_id(cur->parent_id);
  if(parent != NULL)
  {
    for (e = list_begin(&parent->children); e != list_tail(&parent->children); e = list_next(e))
    {
      child = list_entry(e, struct child_status, child_elem);
      
      if(child->child_id == cur->tid)
      {
        lock_acquire(&parent->lock_child);
        child->is_exit_called = true;
        child->exit_status = status;
        lock_release(&parent->lock_child);
      }
    }
  }

  thread_exit();
}

pid_t 
exec (const char *cmd_line)
{
  tid_t tid;
  struct thread *cur = thread_current();

  if(!is_valid_ptr(cmd_line))
    exit(-1);

  cur->child_load_status = 0;
  tid = process_execute(cmd_line);
  lock_acquire(&cur->lock_child);

  while(cur->child_load_status == 0)
    cond_wait(&cur->cond_child, &cur->lock_child);

  if(cur->child_load_status == -1)
    tid = -1;


  lock_release(&cur->lock_child);

  return tid;
}

void 
halt (void)
{
  shutdown_power_off();
}

bool 
create (const char *file_name, unsigned size)
{
  bool status;

  if(!is_valid_ptr(file_name)) exit(-1); // check if valid

  lock_acquire(&fs_lock); // avoid race condition

  status = filesys_create(file_name, size); // file create

  lock_release(&fs_lock);

  return status;
}

bool 
remove (const char *file_name)
{
  bool status;

  if(!is_valid_ptr(file_name)) exit(-1);

  lock_acquire(&fs_lock);

  status = filesys_remove(file_name); // remove file

  lock_release(&fs_lock);

  return status;
}


int 
open (const char *file_name)
{
  int status;
  struct file *f;
  struct file_descriptor *fd;

  if(!is_valid_ptr(file_name)) exit(-1);

  lock_acquire(&fs_lock);

  f = filesys_open(file_name);

  if(f != NULL)
  {

    fd = calloc(1, sizeof(struct file_descriptor*));
    fd->fd_num = allocate_fd();
    fd->file_struct = f;
    list_push_back(&open_files, &fd->elem);
    status = fd->fd_num;

    fd->owner = thread_current()->tid;
  }

  else status = -1;

  lock_release(&fs_lock);

  return status;
}

int 
filesize (int fd)
{
  int status;

  lock_acquire(&fs_lock);

  struct file_descriptor *fd_struct = get_open_file(fd);

  if(fd_struct != NULL)
  {
    status = file_length(fd_struct->file_struct);
  }

  else status = -1;

  return status;
}

struct file_descriptor *
get_open_file (int fd)
{
  struct list_elem *e;
  struct file_descriptor *fd_struct;
  bool match_found = false;

  for (e = list_begin(&open_files); e != list_tail(&open_files); e = list_next(e))
  {
    fd_struct = list_entry(e, struct file_descriptor, elem);

    if (fd_struct->fd_num == fd)
    {
      match_found = true;
      break;
    }
  }

  if(match_found) return fd_struct;
  else return NULL;
}

int 
read (int fd, void *buffer, unsigned size)
{
  int status;
  struct file_descriptor *fd_struct;

  if(!is_valid_ptr(buffer)) exit(-1);

  lock_acquire(&fs_lock);

  if(fd == 1)
  {
    status = -1;
    goto done;
  }

  else if(fd == 0)
  {
    buffer = input_getc();
    status = (int *) buffer;
    goto done;
  }

  else
  {
    fd_struct = get_open_file(fd);
    status = file_read(fd_struct->file_struct, buffer, size);
    goto done;
  }

  done:
    lock_release(&fs_lock);

  return status;
}

int 
write (int fd, const void *buffer, unsigned size)
{
  int status;
  struct file_descriptor *fd_struct;

  if(!is_valid_ptr(buffer)) exit(-1);

  lock_acquire(&fs_lock);

  if(fd == 0)
  {
    status = -1;
    goto done;
  }

  else if(fd == 1)
  {
    putbuf(buffer, size);
    status = size;
    goto done;
  }

  else
  {
    fd_struct = get_open_file(fd);
    status = file_write(fd_struct->file_struct, buffer, size);
    goto done;
  }

  done:
    lock_release(&fs_lock);

  return status;
}

void 
seek (int fd, unsigned position)
{
  lock_acquire(&fs_lock);

  struct file_descriptor *fd_struct = get_open_file(fd);

  if (fd_struct != NULL) file_seek(fd_struct->file_struct, position);

  lock_release(&fs_lock);

}

unsigned 
tell (int fd)
{
  unsigned status;

  lock_acquire(&fs_lock);

  struct file_descriptor *fd_struct = get_open_file(fd);

  if(fd_struct != NULL) status = file_tell(fd_struct->file_struct);

  else status = 0;

  lock_release(&fs_lock);

  return status;
}

void 
close (int fd)
{
  struct thread *cur = thread_current();

  lock_acquire(&fs_lock);

  struct file_descriptor *fd_struct = get_open_file(fd);

  if(fd_struct != NULL) 
  {
    if(cur->tid == fd_struct->owner)
      close_open_file(fd);
  }

  lock_release(&fs_lock);
}

void 
close_open_file (int fd)
{
  struct list_elem *e;
  struct file_descriptor *fd_struct;

  for (e = list_begin(&open_files); e != list_tail(&open_files); e = list_next(e))
  {
    fd_struct = list_entry(e, struct file_descriptor, elem);

    if(fd_struct->fd_num == fd)
    {
      list_remove(e);
      file_close(fd_struct->file_struct);
      free(fd_struct);
      break;
    }
  }
}

static int
allocate_fd(void)
{
  return fs_id++;
}

void 
close_file_by_owner(tid_t tid)
{
  struct list_elem *e;
  struct file_descriptor *fd_struct;

  for (e = list_begin(&open_files); e != list_tail(&open_files); e = list_next(e))
  {
    fd_struct = list_entry(e, struct file_descriptor, elem);
    if(fd_struct->owner == tid)
    {
      list_remove(e);
      file_close(fd_struct->file_struct);
      free(fd_struct);
    }
  }
}
