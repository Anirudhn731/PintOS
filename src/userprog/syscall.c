#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"

int exit_code = 0;
static void syscall_handler (struct intr_frame *);

void syscall_exit(void) {
  printf("%s: exit(%d)\n", thread_current()->name, exit_code);
  thread_exit();
}

void validate_safe_memory(const void *vaddr) {
  if(!is_user_vaddr(vaddr)) {
    exit_code = 1;
    syscall_exit();
  }

  const void *p = pagedir_get_page(thread_current()->pagedir, vaddr);
  if(p == NULL) {
    exit_code = 2;
    syscall_exit();
  }
}

void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
  int* f_esp = f->esp;
  int syscall = *f_esp;
  switch (syscall) {
    case SYS_HALT:
      shutdown_power_off();
      break;

    case SYS_WRITE:
      validate_safe_memory(*(f_esp+2));
      if(*(f_esp+1) == 1) putbuf(*(f_esp+2), *(f_esp+3));
      break;

    case SYS_EXIT:
      syscall_exit();
      break;

    default:
      printf("Syscall :- Confused Call\n");
  }
}
