#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

void syscall_init (void);
void syscall_exit(int exit_status);
// void syscall_close(struct list*, int);
void validate_safe_memory(const void *vaddr);

#endif /* userprog/syscall.h */
