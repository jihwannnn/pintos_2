#ifndef THREADS_THREAD_H
#define THREADS_THREAD_H

#include <debug.h>
#include <list.h>
#include <stdint.h>

/* States in a thread's life cycle. */
enum thread_status
  {
    THREAD_RUNNING,     /* Running thread. */
    THREAD_READY,       /* Not running but ready to run. */
    THREAD_BLOCKED,     /* Waiting for an event to trigger. */
    THREAD_DYING        /* About to be destroyed. */
  };

/* Thread identifier type.
   You can redefine this to whatever type you like. */
typedef int tid_t;
#define TID_ERROR ((tid_t) -1)          /* Error value for tid_t. */

/* Thread priorities. */
#define PRI_MIN 0                       /* Lowest priority. */
#define PRI_DEFAULT 31                  /* Default priority. */
#define PRI_MAX 63                      /* Highest priority. */

/* A kernel thread or user process.

   Each thread structure is stored in its own 4 kB page.  The
   thread structure itself sits at the very bottom of the page
   (at offset 0).  The rest of the page is reserved for the
   thread's kernel stack, which grows downward from the top of
   the page (at offset 4 kB).  Here's an illustration:

        4 kB +---------------------------------+
             |          kernel stack           |
             |                |                |
             |                |                |
             |                V                |
             |         grows downward          |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             |                                 |
             +---------------------------------+
             |              magic              |
             |                :                |
             |                :                |
             |               name              |
             |              status             |
        0 kB +---------------------------------+

   The upshot of this is twofold:

      1. First, `struct thread' must not be allowed to grow too
         big.  If it does, then there will not be enough room for
         the kernel stack.  Our base `struct thread' is only a
         few bytes in size.  It probably should stay well under 1
         kB.

      2. Second, kernel stacks must not be allowed to grow too
         large.  If a stack overflows, it will corrupt the thread
         state.  Thus, kernel functions should not allocate large
         structures or arrays as non-static local variables.  Use
         dynamic allocation with malloc() or palloc_get_page()
         instead.

   The first symptom of either of these problems will probably be
   an assertion failure in thread_current(), which checks that
   the `magic' member of the running thread's `struct thread' is
   set to THREAD_MAGIC.  Stack overflow will normally change this
   value, triggering the assertion. */
/* The `elem' member has a dual purpose.  It can be an element in
   the run queue (thread.c), or it can be an element in a
   semaphore wait list (synch.c).  It can be used these two ways
   only because they are mutually exclusive: only a thread in the
   ready state is on the run queue, whereas only a thread in the
   blocked state is on a semaphore wait list. */
struct thread
  {
    /* Owned by thread.c. */
    tid_t tid;                          /* Thread identifier. */
    enum thread_status status;          /* Thread state. */
    char name[16];                      /* Name (for debugging purposes). */
    uint8_t *stack;                     /* Saved stack pointer. */
    int priority;                       /* Priority. */
    struct list_elem allelem;           /* List element for all threads list. */

   /* for alarm*/
   int64_t local_tick;                  /* tick for wake up */

   int init_priority;                   /* for keep original priority */
   struct lock *wait_on_lock;           /* lock that thread is waiting for get */
   struct list donations;               /* list of threads that donate their priority */
   struct list_elem donation_elem;      /* element for manage the list donations */

   int nice;                            /* value for nice */
   int recent_cpu;                      /* value for recent_cpu */

    /* Shared between thread.c and synch.c. */
    struct list_elem elem;              /* List element. */

#ifdef USERPROG
    /* Owned by userprog/process.c. */
    uint32_t *pagedir;                  /* Page directory. */
    
    // pt 2-2, 추가 선언

    // pt 2-2, thread id of parent
    tid_t parent_id;

    /* pt 2-2, signal to indicate the child's executable-loading status 
    * 0: has not been loaded
    * -1: load failed
    * 1: load successed */
    int child_load_status;

    /* pt 2-2, monitor used to wait the child, owned by wait-syscall
    * and waiting for child t load executable */
    struct lock *lock_child;
    struct condition *cond_child;

    /* pt 2-2,  list of children, which should be a list of struct child_status */
    struct list children;

    /* pt 2-2, file struct represents the execuatable of the current thread */
    struct file *exec_file;

    // pt 2-2 check if exit is called for the thread
#endif

    /* Owned by thread.c. */
    unsigned magic;                     /* Detects stack overflow. */
  };

// pt 2-2 child status
struct child_status 
   {
      tid_t child_id;
      bool is_exit_called;
      bool has_been_waited;
      int child_exit_status;
      struct list_elem elem_child_status;  
   };

/* If false (default), use round-robin scheduler.
   If true, use multi-level feedback queue scheduler.
   Controlled by kernel command-line option "-o mlfqs". */
extern bool thread_mlfqs;

void thread_init (void);
void thread_start (void);

void thread_tick (void);
void thread_print_stats (void);

typedef void thread_func (void *aux);
tid_t thread_create (const char *name, int priority, thread_func *, void *);

void thread_block (void);
void thread_unblock (struct thread *);

struct thread *thread_current (void);
tid_t thread_tid (void);
const char *thread_name (void);

void thread_sleep(int64_t ticks);
void thread_wake_up(int64_t ticks);

bool thread_compare_priority (struct list_elem *list_elem, struct list_elem *e, void *aux);
void thread_test_preemption (void);

bool thread_compare_donate_priority (const struct list_elem *l, const struct list_elem *s, void *aux);

void donate_priority (void);

void remove_with_lock (struct lock *lock);
void refresh_priority (void);

void calculate_priority (struct thread *t);
void calculate_recent_cpu (struct thread *t);
void calculate_load_avg (void);

void increase_recent_cpu (void);
void recalculate_all_priority (void);
void recalculate_all_recent_cpu (void);

// get global tick
int64_t get_global_tick(void);

/* Performs some operation on thread t, given auxiliary data AUX. */
typedef void thread_action_func (struct thread *t, void *aux);
void thread_foreach (thread_action_func *, void *);

int thread_get_priority (void);
void thread_set_priority (int);

int thread_get_nice (void);
void thread_set_nice (int nice);
int thread_get_recent_cpu (void);
int thread_get_load_avg (void);

// pt 2-2 thread_get_by_id
struct thread *thread_get_by_id (tid_t id);

#endif /* threads/thread.h */
