#include "userprog/syscall.h"
#include <stdio.h>
#include <stdlib.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "userprog/pagedir.h"
#include "userprog/process.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "devices/input.h"

int exit_code = 0;
struct mapping_file {
  int fd;
  struct file* file_;
  struct list_elem elem;
};

struct mapping_file* files_search(struct list* files, int fd) {
  for (struct list_elem *e = list_begin(files); e != list_end(files); e = list_next(e)) {
    struct mapping_file* temp_file = list_entry(e, struct mapping_file, elem);
    if(temp_file->fd == fd) {
      return temp_file;
    }
  }
  return NULL;
}

static void syscall_handler (struct intr_frame *);

void syscall_exit(int exit_status) {
  for(struct list_elem* e = list_begin(&(thread_current()->parent_thread->child_threads)); e != list_end(&(thread_current()->parent_thread->child_threads)); e = list_next(e)) {
    struct child_thread* temp_child = list_entry(e, struct child_thread, elem);
    if(temp_child) {
      if(temp_child->child_tid == thread_current()->tid) {
        temp_child->is_exit = true;
  
        lock_acquire(&(thread_current()->parent_thread->lock_child));
        if(thread_current()->parent_thread->locked_child_tid == thread_current()->tid) {
          cond_signal(&(thread_current()->parent_thread->cond_child), &(thread_current()->parent_thread->lock_child));
        }
        lock_release(&(thread_current()->parent_thread->lock_child));
        break;
      }
    }
    else {
      exit_code = -1;
    }
  }

  exit_code = exit_status;
  printf("%s: exit(%d)\n", thread_current()->name, exit_code);
  thread_exit();
}

void syscall_close(struct list* files, int fd) {
  lock_acquire_filesys();
  for(struct list_elem *e = list_begin(files); e != list_end(files); e = list_next(e)) {
    struct mapping_file* temp_file = list_entry(e, struct mapping_file, elem);
    if(temp_file->fd == fd) {
      file_close(temp_file->file_);
      list_remove(e);
    }
  }
  lock_release_filesys();
}

int syscall_exec(char* file_name) {
  lock_acquire_filesys();
  char* file_name_copy = malloc((strlen(file_name) + 1) * sizeof(char));
  strlcpy(file_name_copy, file_name, strlen(file_name) + 1);
  char* rest;
  file_name_copy = strtok_r(file_name_copy, " ", &rest);

  struct file* open_file = filesys_open(file_name_copy);
  if(open_file != NULL) {
    file_close(open_file);
    lock_release_filesys();
    return process_execute(file_name);
  }
  else {
    lock_release_filesys();
    return -1;
  }

}

void close_all_files(struct list* files)
{
	struct list_elem *e;
      for (e = list_begin (files); e != list_end (files);
           e = list_next (e))
        {
          struct mapping_file *f = list_entry (e, struct mapping_file, elem);
          
	      	file_close(f->file_);
	      	list_remove(e);
        }
}

void validate_safe_memory(const void *vaddr) {
  if(!is_user_vaddr(vaddr) || !pagedir_get_page(thread_current()->pagedir, vaddr)) {
    syscall_exit(-1);
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
  validate_safe_memory(f_esp);
  int syscall = *f_esp;
  switch (syscall) {
    case SYS_HALT:
      shutdown_power_off();
      break;

    case SYS_EXEC:
      validate_safe_memory(f_esp + 1);
      validate_safe_memory(*(f_esp + 1));
      f->eax = syscall_exec(*(f_esp+1));
      break;

    case SYS_READ:
      validate_safe_memory(f_esp + 7);
      validate_safe_memory(*(f_esp + 6));
      if(*(f_esp + 5) == 0) {
        uint8_t* buffer = *(f_esp + 6);
        int buffer_size = *(f_esp + 7);
        for (int i = 0; i < buffer_size; i++) {
          buffer[i] = input_getc();
        }

        f->eax = buffer_size;
      }
      else {
        struct mapping_file* read_file = files_search(&thread_current()->files, *(f_esp+5));
        if(read_file != NULL) {
          lock_acquire_filesys();
          f->eax = file_read_at(read_file->file_, *(f_esp + 6), *(f_esp + 7), 0);
          lock_release_filesys();
        }
        else 
          f->eax = -1;
      }
      break;

    case SYS_WRITE:
      validate_safe_memory(f_esp + 7);
      validate_safe_memory(*(f_esp + 6));
      if(*(f_esp+5) == 1) {
        putbuf(*(f_esp+6), *(f_esp+7));
        f->eax = *(f_esp + 7);
      }
      else {
        struct mapping_file* write_file = files_search(&thread_current()->files, *(f_esp+5));
        if(write_file != NULL) {
          lock_acquire_filesys();
          f->eax = file_write_at(write_file->file_, *(f_esp + 6), *(f_esp + 7), 0);
          lock_release_filesys();
        }
        else
          f->eax = -1;
      }
      break;

    case SYS_EXIT:
      validate_safe_memory(f_esp + 1);
      syscall_exit(*(f_esp+1));
      break;

    case SYS_CREATE:
      validate_safe_memory(f_esp + 5);
      validate_safe_memory(*(f_esp + 4));
      lock_acquire_filesys();
      f->eax = filesys_create(*(f_esp+4), *(f_esp+5));
      lock_release_filesys();
      break;

    case SYS_OPEN:
      validate_safe_memory(f_esp + 1);
      validate_safe_memory(*(f_esp+1));
      lock_acquire_filesys();
      struct file* open_file = filesys_open(*(f_esp+1));
      lock_release_filesys();
      if(open_file == NULL) {
        f->eax = -1;
      }
      else {
        struct mapping_file* mapping_open_file = malloc(sizeof(*mapping_open_file));
        
        mapping_open_file->fd = thread_current()->next_fd++;
        mapping_open_file->file_ = open_file;
        list_push_back(&(thread_current()->files), &(mapping_open_file->elem));
        
        f->eax = mapping_open_file->fd;
      }

      break;

    case SYS_CLOSE:
      validate_safe_memory(f_esp + 1);
      syscall_close(&thread_current()->files, *(f_esp+1));
      break;

    case SYS_REMOVE:
      validate_safe_memory(f_esp + 1);
      validate_safe_memory(*(f_esp+1));
      lock_acquire_filesys();
      f->eax = filesys_remove(*(f_esp + 1));
      lock_release_filesys();
      break;

    case SYS_FILESIZE:
      validate_safe_memory(f_esp + 1);
      lock_acquire_filesys();
      f->eax = file_length(files_search(&thread_current()->files, *(f_esp + 1))->file_);
      lock_release_filesys();
      break;

    case SYS_WAIT:
      validate_safe_memory(f_esp + 1);
      f->eax = process_wait(*(f_esp + 1));
      break;

    case SYS_SEEK:
      validate_safe_memory(f_esp + 5);
      lock_acquire_filesys();
      file_seek(files_search(&(thread_current()->files), *(f_esp+4))->file_, *(f_esp+5));
      lock_release_filesys();
      break;

    case SYS_TELL:
      validate_safe_memory(f_esp + 1);
      lock_acquire_filesys();
      f->eax = file_tell(files_search(&thread_current()->files, *(f_esp + 1))->file_);
      lock_release_filesys();
      break;

    default:
      printf("%d\n", *f_esp);
  }
}
