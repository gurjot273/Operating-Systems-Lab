#ifndef USERPROG_SYSCALL_H
#define USERPROG_SYSCALL_H

#include <stdbool.h>
#include "threads/thread.h"
#include <list.h>
#include "threads/synch.h"


struct lock file_lock;       /*structure to lock an unlock access file with multi thread*/

struct fd_elem
{
    int fd;                        /*used to store the file descriptors ID*/
    struct file *myfile;           /* used to store the real file*/
    struct list_elem elem;      /*list elem to add fd_elem in fd_list*/
};

struct fd_elem* extract_fd(int fd);
void syscall_init (void);
void sys_exit (int status);


#endif /* userprog/syscall.h */
