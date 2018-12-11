/*********************************************************
 * Your Name: Parker Zimmerman
 * Partner Name: Yina Wang
 *********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/sysinfo.h>

#include "thread.h"

#define BUFF_SIZE (4*1024)

// Data structure to hold both the character and its frequency
struct combo {
    int Num;
    char C;
};

// constructor for the combo struct
inline struct combo* combo(int* a)
{
    return (struct combo*)(a);
}

// Data structure to hold the lock which will be used 
struct file {
    FILE * file; 
    char *buffer; 
    int num_read_threads;
    int num_zip_threads;
    int chunk_sizes;
    struct lock f_lock;
    struct condition finished_reading;
    struct condition finished_writing;
    struct condition finished_file;    
};

struct thread_control {
    int thread_num;
    struct file Fl;
};

inline struct thread_control* thread_control(int n, struct file fl){
    struct thread_control thrd_cntrl;
    thrd_cntrl->thread_num = n;
    thrd_cntrl->Fl = fl;
}

// initializing the file conditions and locks
void pzip_init(struct file *fl){
    fl->buffer = (char*)malloc (BUFF_SIZE*sizeof(char));
    lock_init(&fl->f_lock);
    cond_init(&fl->finished_reading);
    cond_init(&fl->finished_writing);
    cond_init(&fl->finished_file);
}

// error checking for file opening
FILE *openFile(char const* path){
  FILE* f = fopen(path,"rb");
  if (f){
      fseek(f, 0, SEEK_END); // seek to end of file
      size_t size = ftell(f); // get current file pointer
      fseek(f, 0, SEEK_SET); // seek back to beginning of file
      f = mmap(NULL,size,(PROT_READ|PROT_WRITE),MAP_SHARED,&f,0);
    return f;
  } else{
    fputs("file open failed",stderr),exit(1);
  }
}

//reading the file in 4k chunks
void* readFile(void *arg) {
    struct file *fl = (struct file*)arg; 
    //lock_acquire(&file->f_lock);
    char *name = (char *)malloc(2*sizeof(char));
    //pthread_getname_np(pthread_self(), name, 2);
    int t_num = (int)name[0];
    int starting_pos = t_num*fl->chunk_sizes;
    //size_t offset = starting_pos * sizeof(char);
    //fsetpos(fl->file,offset);
    if (&fl->buffer) {
        if (0 == fread(&fl->buffer[starting_pos], sizeof(char), fl->chunk_sizes, fl->file)){
             fputs("file read failed",stderr),exit(1);
        }
    }
    free(name);
    //signals when the file is done reading 
    //cond_signal(&file->finished_reading, &file->f_lock);
    //lock_release(&file->f_lock);            
    
}

// parallelized zipping 
int pzip(struct file *fl) {
    lock_acquire(&fl->f_lock);
    // int n = 0;
    // char lastChar = -1;
    // int numOccur = 0;
    // // store the contents into an array
    // while (file) {
    //     if (!numOccur) {
    //         numOccur = 1;
    //         lastChar = file->buffer[0];
    //  }
    //}
    fwrite(&fl->buffer, sizeof(char), BUFF_SIZE, stdout);
    lock_release(&fl->f_lock);
   
           
    return 0;
}

pthread_t *createThreads(struct file *fl, int num, void * func, pthread_t *rthrd){
    lock_acquire(&fl->f_lock);
    char fun;
    if (func == readFile){
        fun = 'r';
        fl->num_read_threads = num;
    } else if (func == pzip){
        fun = 'p';
        fl->num_zip_threads = num;
    }
    for (int n = 1; n <= num; n++) {
        pthread_t tid;
        int ret = pthread_create(&tid, NULL, func, &fl);
        int set = pthread_setname_np(&tid,"%d%d",n-1,fun);
        if (ret != 0 || set != 0) {
            perror("pthread_create");
            exit(1);
        rthrd[n-1] = tid;
    }
    lock_release(&fl->f_lock);
}

void manageThreads(struct file *fl) {
    lock_acquire(&fl->f_lock);
    int num = get_nprocs();
    fl->num_read_threads = num - 1;
    fl->num_zip_threads = 1;
    fl->chunk_sizes = BUFF_SIZE/fl->num_read_threads;
    pthread_t *rthrd = (pthread_t *)malloc([fl->num_read_threads] * sizeof(pthread_t));
    createThreads(fl, fl->num_read_threads, readFile, rthrd);
    for (int n=0; n<fl->num_read_threads;n++){
        pthread_join(rthrd[n], null);
    }
    // once all read threads have terminated the buffer has been filled
    // and we can write to stdout
    createThreads(fl,fl->num_zip_threads,pzip);
    lock_release(&fl->f_lock);
}

int main(int argc, char *argv[]){

    struct file fl;
	pzip_init(&fl);

     fl->file = openFile(argv[1]);
    if (get_nprocs > 0) {
        createThreads(fl, 1, manageThreads)
    }

    return 0;
}
/*
cant print in parallel
break the file into certain size chunks based on memory usage
break those chunks up based on number of threads
mult threads working on reading file at once
then use one thread to print chunk
move onto next chunk once done
*/