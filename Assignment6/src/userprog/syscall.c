#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/vaddr.h"
#include "threads/malloc.h"
#include "devices/shutdown.h"
#include "devices/input.h"
#include "process.h"
#include "filesys/file.h"
#include "filesys/filesys.h"
#include "threads/synch.h"

static void syscall_handler (struct intr_frame *);


void check_valid_ptr (const void *ptr)
{
    struct thread* cur=thread_current();
    if(!ptr || !is_user_vaddr(ptr) || !pagedir_get_page(cur->pagedir,ptr))
        sys_exit(-1);
}

void sys_halt (void)
{
    shutdown_power_off();
}


void sys_exit (int status)
{
    struct thread *cur = thread_current();
    printf ("%s: exit(%d)\n", cur -> name, status);

    // finding the child element
    struct child_elem *child = extract_child(cur->tid, &cur -> parent -> child_list);
    if(child!=NULL){
	    // exit status of child is being set
	child -> exit_status = status;
	    // current status of child is being marked
	child -> cur_status =(status==-1)?WAS_KILLED:HAD_EXITED;
    }
    thread_exit();
}

tid_t sys_exec (const char *cmd_line)
{
    return process_execute(cmd_line);
}

int sys_wait (tid_t pid)
{
    return process_wait(pid);
}

bool sys_create (const char *file, unsigned initial_size)
{
    lock_acquire(&file_lock);
    bool retVal = filesys_create(file, initial_size);
    lock_release(&file_lock);
    return retVal;
}

bool sys_remove (const char *file)
{
    lock_acquire(&file_lock);
    bool retVal = filesys_remove(file);
    lock_release(&file_lock);
    return retVal;
}

int sys_open (const char *file)
{
    lock_acquire(&file_lock);
    struct file * open_fl = filesys_open(file);
    if(open_fl)
    {
        thread_current ()->fd_size = thread_current ()->fd_size + 1;
        int retVal = thread_current ()->fd_size;
        /*new fd_elem is created and initialized */
        struct fd_elem *fd_el = (struct fd_elem*) malloc(sizeof(struct fd_elem));
        fd_el->fd = thread_current()->fd_size;
        fd_el->myfile = open_fl;
        // fd_elem is added to the thread's fd_list
        list_push_back(&thread_current()->fd_list, &fd_el->elem);
        lock_release(&file_lock);
        return retVal;
    }
    lock_release(&file_lock);
    return -1;
}

int sys_filesize (int fd)
{
    lock_acquire(&file_lock);
    struct fd_elem *fd_el = extract_fd(fd); 
	if(!fd_el)
	{
	    lock_release(&file_lock);
	    return -1;
	}
    int ret = file_length(fd_el->myfile);
    lock_release(&file_lock);
    return ret;
}

int sys_read (int fd, void *buffer, unsigned size)
{
    lock_acquire(&file_lock);
    if(fd == 0)
    {
        int i=0;
        char* buf = (char *)buffer;
	for(i=0;i<size;i++)
		buf[i] = input_getc();
        lock_release(&file_lock);
        return (int)size;
    }
    else if(fd > 0)
    {
        //file is being read
        //fd_elem is extracted
        struct fd_elem *fd_el = extract_fd(fd);
        if(!fd_el){
            lock_release(&file_lock);
            return -1;
        }
        int retVal = file_read(fd_el->myfile,buffer,size);
        lock_release(&file_lock);
        return retVal;
    }
    lock_release(&file_lock);
    return -1;
}

int sys_write (int fd, const void *buf, unsigned size)
{
    lock_acquire(&file_lock);
    if (fd == 1)
    {
        putbuf( (char *)buf, size); // written to the console
        lock_release(&file_lock);
        return (int)size;
    }
    else
    {
        //file is being written
        //fd_elem is being extracted
        struct fd_elem *fd_el = extract_fd(fd);
        if(!fd_el)
        {
            lock_release(&file_lock);
            return -1;
        }
        int retVal = file_write(fd_el->myfile,buf,size);
        lock_release(&file_lock);
        return retVal;
    }
    return -1;
}


void sys_seek (int fd, unsigned position)
{
    lock_acquire(&file_lock);
    struct fd_elem *fd_el = extract_fd(fd);
    if(!fd_el)
    {
        lock_release(&file_lock);
        return;
    }
    file_seek(fd_el->myfile,position);
    lock_release(&file_lock);
}

unsigned sys_tell (int fd)
{
    lock_acquire(&file_lock);
    struct fd_elem *fd_el = extract_fd(fd);
    if(!fd_el)
    {
        lock_release(&file_lock);
        return -1;
    }
    unsigned retVal = file_tell(fd_el->myfile);
    lock_release(&file_lock);
    return retVal;
}

void sys_close (int fd)
{
    lock_acquire(&file_lock);
    struct list_elem *e;
    for (e = list_begin (&thread_current()->fd_list); e != list_end (&thread_current()->fd_list);
            e = list_next (e))
    {
        struct fd_elem *fd_elem = list_entry (e, struct fd_elem, elem);
        if(fd_elem->fd == fd)
        {
            list_remove(e);
	    file_close(fd_elem->myfile);
	    free(fd_elem);
	    lock_release(&file_lock);
            return;
        }
    }
    lock_release(&file_lock);
}


void
syscall_init (void)
{
    intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
    lock_init(&file_lock);
}

static void
syscall_handler (struct intr_frame *f )
{
    check_valid_ptr(f->esp);
    int sys_num = *((int *)f->esp);
    if(sys_num==SYS_HALT){
        sys_halt();
    }
    else if(sys_num==SYS_EXIT){
	check_valid_ptr(f->esp+4);
	int argv = *((int*)(f->esp+4));
	sys_exit(argv);
    }
    else if(sys_num==SYS_EXEC){
	check_valid_ptr(f->esp+4);
	char * argv = *((char **)(f->esp+4));
        check_valid_ptr((const void*) argv);
        f -> eax = sys_exec(argv);
    }
    else if(sys_num==SYS_WAIT){
	check_valid_ptr(f->esp+4);
	int argv = *((int*)(f->esp+4));
	f-> eax = sys_wait(argv);
    }
    else if(sys_num==SYS_CREATE){
	check_valid_ptr(f->esp+4);
	char * argv = *((char **)(f->esp+4));
	check_valid_ptr(f->esp+8);
	unsigned argv_1 = *((unsigned *)(f->esp+8));
        check_valid_ptr((const void*) argv);
        f -> eax = sys_create(argv,argv_1);
    }
    else if(sys_num==SYS_REMOVE){
	check_valid_ptr(f->esp+4);
	char * argv = *((char **)(f->esp+4));
        check_valid_ptr((const void*) argv);
        f -> eax = sys_remove(argv);
    }
    else if(sys_num==SYS_OPEN){
	check_valid_ptr(f->esp+4);
	char * argv = *((char **)(f->esp+4));
        check_valid_ptr((const void*) argv);
        f -> eax = sys_open(argv);
    }
    else if(sys_num==SYS_FILESIZE){
	check_valid_ptr(f->esp+4);
	int argv = *((int*)(f->esp+4));
	f-> eax = sys_filesize(argv);
    }
    else if(sys_num==SYS_READ){
	check_valid_ptr(f->esp+4);
	int argv = *((int*)(f->esp+4));
	check_valid_ptr(f->esp+8);
	void * argv_1 = *((void **)(f->esp+8));
	check_valid_ptr(f->esp+12);
	unsigned argv_2 = *((unsigned *)(f->esp+12));
	check_valid_ptr((const void*) argv_1);
	void * temp = argv_1+((int)argv_2-1) ;
	check_valid_ptr((const void*) temp);
	f->eax = sys_read (argv,argv_1,argv_2);
    }
    else if(sys_num==SYS_WRITE){
	check_valid_ptr(f->esp+4);
	int argv = *((int*)(f->esp+4));
	check_valid_ptr(f->esp+8);
	void * argv_1 = *((void **)(f->esp+8));
	check_valid_ptr(f->esp+12);
	unsigned argv_2 = *((unsigned *)(f->esp+12));
	check_valid_ptr((const void*) argv_1);
	void * temp = argv_1+((int)argv_2-1) ;
	check_valid_ptr((const void*) temp);
	f->eax = sys_write (argv,argv_1,argv_2);
    }
    else if(sys_num==SYS_SEEK){
	check_valid_ptr(f->esp+4);
	int argv = *((int*)(f->esp+4));
	check_valid_ptr(f->esp+8);
	unsigned argv_1 = *((unsigned *)(f->esp+8));
        sys_seek(argv,argv_1);
    }
    else if(sys_num==SYS_TELL){
	check_valid_ptr(f->esp+4);
	int argv = *((int*)(f->esp+4));
	f-> eax = sys_tell(argv);
    }
    else if(sys_num==SYS_CLOSE){
	check_valid_ptr(f->esp+4);
	int argv = *((int*)(f->esp+4));
	sys_close(argv);
    }
    else{
	sys_exit(-1);
    }
}

struct fd_elem* extract_fd(int fd)
{
    struct list_elem *e=list_begin (&thread_current()->fd_list);
    while(e!= list_end (&thread_current()->fd_list)){
	struct fd_elem *fd_elem = list_entry (e, struct fd_elem, elem);
	if(fd_elem->fd == fd)
	{
	    return fd_elem;
	}
    }
    return NULL;
}
