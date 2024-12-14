#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"

static void syscall_handler (struct intr_frame *);

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
      if(*(f_esp+1) == 1) putbuf(*(f_esp+2), *(f_esp+3));
      break;

    case SYS_EXIT:
      printf("%s: exit(%d)\n", thread_current()->name, 0);
      thread_exit();
      break;

    default:
      printf("Syscall :- Confused Call\n");
  }
}
