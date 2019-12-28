#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <devices/shutdown.h>
#include <threads/vaddr.h>
#include <filesys/filesys.h>
#include <string.h>
#include <filesys/file.h>
#include <devices/input.h>
#include <threads/palloc.h>
#include <threads/malloc.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "process.h"
#include "pagedir.h"

static void syscall_handler (struct intr_frame *);
static void verity_address_multiple(int * p, int num);
static void verity_address(void * p);
struct opened_file * search_file(int fd);

void
syscall_init (void)
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

void unexpected_exit(){
  thread_current()->exit_status = -1;
  thread_exit ();
}


void verity_address_multiple(int *p, int num){
  int *p2=p;
  int i;
  for(i=0; i<num; i++,p2++) verity_address((void *)p2);
}

void verity_address(void *p){
  if (p==NULL) unexpected_exit();
  if(!is_user_vaddr(p)) unexpected_exit();
  if(!is_user_vaddr(p+4)) unexpected_exit();
  if(!pagedir_get_page(thread_current()->pagedir,p)) unexpected_exit();

}

struct opened_file * search_file(int fd){
  struct list_elem *e;
  struct opened_file * opf =NULL;
  struct list *files = &thread_current()->files;
  for (e = list_begin (files); e != list_end (files); e = list_next (e)){
    opf = list_entry (e, struct opened_file, file_elem);
    if (fd==opf->fd)
      return opf;
  }
  return false;
}


static void
syscall_handler (struct intr_frame *f UNUSED)
{
  int * p =f->esp;
  verity_address((void *)p);
  int type=*p++;

  switch (type){
    case SYS_HALT:
      shutdown_power_off();

    case SYS_EXIT:
      verity_address((void *)p);
      thread_current()->exit_status = *p;
      thread_exit ();

    case SYS_EXEC:
      verity_address((void *)p);
      verity_address((void*)*p);
      f->eax = process_execute((char*)*p);
      break;

    case SYS_WAIT:
      verity_address((void *)p);
      f->eax = process_wait(*p);
      break;

    case SYS_CREATE:
      verity_address_multiple(p, 2);
      verity_address((void*)*p);
      acquire_file_lock();
      f->eax = filesys_create((const char *)*p,*(p+1));
      release_file_lock();
      break;

    case SYS_REMOVE:
      verity_address((void *)p);
      verity_address((void*)*p);
      acquire_file_lock();
      f->eax = filesys_remove((const char *)*p);
      release_file_lock();
      break;

    case SYS_OPEN:
      verity_address((void *)p);
      verity_address((void*)*p);
      struct thread * t=thread_current();
      acquire_file_lock();
      struct file * thefile =filesys_open((const char *)*p);
      release_file_lock();
      if(thefile){
        struct opened_file *of = malloc(sizeof(struct opened_file));
        of->fd = t->next_fd++;
        of->file = thefile;
        list_push_back(&t->files, &of->file_elem);
        f->eax = of->fd;
      } else
        f->eax = -1;
      break;

    case SYS_FILESIZE:
      verity_address((void *)p);
      struct opened_file * thefile2 = search_file(*p);
      if (thefile2){
        acquire_file_lock();
        f->eax = file_length(thefile2->file);
        release_file_lock();
      } else
        f->eax = -1;
      break;

    case SYS_READ:
      verity_address_multiple(p, 3);
      verity_address((void*)*(p+1));
      int fd = *p;
      uint8_t * buffer = (uint8_t*)*(p+1);
      off_t size = *(p+2);
      if (fd==0) {
        int i;
        for (i=0; i<size; i++)
          buffer[i] = input_getc();
        f->eax = size;
      }
      else{
        struct opened_file * thefile3 = search_file(*p);
        if (thefile3){
          acquire_file_lock();
          f->eax = file_read(thefile3->file, buffer, size);
          release_file_lock();
        } else
          f->eax = -1;
      }
      break;

    case SYS_WRITE:
      verity_address_multiple(p, 3);
      verity_address((void*)*(p+1));
      int fd2 = *p;
      const char * buffer2 = (const char *)*(p+1);
      off_t size2 = *(p+2);;
      if (fd2==1) {
        putbuf(buffer2,size2);
        f->eax = size2;
      }
      else{
        struct opened_file * thefile4 = search_file(*p);
        if (thefile4){
          acquire_file_lock();
          f->eax = file_write(thefile4->file, buffer2, size2);
          release_file_lock();
        } else
          f->eax = 0;
      }
      break;

    case SYS_SEEK:
      verity_address_multiple(p, 2);
      struct opened_file * thefile5 = search_file(*p);
      if (thefile5){
        acquire_file_lock();
        file_seek(thefile5->file, *(p+1));
        release_file_lock();
      }
      break;

    case SYS_TELL:
      verity_address((void *)p);
      struct opened_file * thefile6 = search_file(*p);
      if (thefile6){
        acquire_file_lock();
        f->eax = file_tell(thefile6->file);
        release_file_lock();
      }else
        f->eax = -1;
      break;

    case SYS_CLOSE:
      verity_address((void *)p);
      struct opened_file * openf=search_file(*p);
      if (openf){
        acquire_file_lock();
        file_close(openf->file);
        release_file_lock();
        list_remove(&openf->file_elem);
        free(openf);
      }
      break;

    default:
      unexpected_exit();
      break;
  }
}
