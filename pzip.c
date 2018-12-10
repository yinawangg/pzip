/*********************************************************
 * Your Name: Parker Zimmerman
 * Partner Name: Yina Wang
 *********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

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

// initializing the file conditions and locks
void pzip_init(struct file *file){
    file->buffer = (char*)malloc (BUFF_SIZE*sizeof(char));
    lock_init(&file->f_lock);
    cond_init(&file->finished_reading);
    cond_init(&file->finished_writing);
    cond_init(&file->finished_file);
}

// error checking for file opening
FILE *openFile(char const* path){
  FILE* f = fopen(path,"rb");
  if (f){
    return f;
  } else{
    fputs("file open failed",stderr),exit(1);
  }
}

//reading the file in 4k chunks
void* readFile(void *arg) {
    struct file *file = (struct file*)arg; 
    lock_acquire(&file->f_lock);
    char name[2];
    pthread_getname_np(pthread_self(), name, 2);
    int t_num = (int)name[0];
    int starting_pos = t_num*file->chunk_sizes;
    size_t offset = starting_pos * sizeof(char);
    int n;
    fsetpos(file->file,offset);
    if (&file->buffer) {
        while((n = fread(&file->buffer[starting_pos], sizeof(char), file->chunk_sizes, &file->file)));
    }
    free(name);
    //signals when the file is done reading 
    cond_signal(&file->finished_reading, &file->f_lock);
    lock_release(&file->f_lock);            
    
}

// parallelized zipping 
int pzip(struct file *file) {
    lock_acquire(&file->f_lock);
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
    fwrite(&file->buffer, sizeof(char), BUFF_SIZE, stdout);
    lock_release(&file->f_lock);
   
           
    return 0;
}

void createThreads(struct file *file, int num, void * func){
    lock_acquire(&file->f_lock);
    char f;
    if (func == readFile){
        f = 'r';
        file->num_read_threads = num;
    } else if (func == pzip){
        f = 'p';
        file->num_zip_threads = num;
    }
    for (int n = 1; n <= num; n++) {
        pthread_t tid;
            int ret = pthread_create(&tid, NULL, func, &file);
            int set = pthread_setname_np(&tid,"%d%d",n-1,f);
            if (ret != 0 || set != 0) {
                perror("pthread_create");
                exit(1);
    }
    lock_release(&file->f_lock);
}

void manageThreads(struct file *file) {
    lock_acquire(&file->f_lock);
    int num = get_nprocs();
    file->num_read_threads = num - 1;
    file->num_zip_threads = 1;
    file->chunk_sizes = BUFF_SIZE/file->num_read_threads;
    createThreads(file, file->num_read_threads, readFile);
    for (int n=0; n<file->num_read_threads;n++){
        cond_wait(&file->finished_reading, &file->f_lock);
    }
    // once all read threads have terminated the buffer has been filled
    // and we can write to stdout
    createThreads(file,file->num_zip_threads,pzip);
    lock_release(&file->f_lock);
}

int main(int argc, char *argv[]){

    struct file file;
	pzip_init(&file);

    FILE *file = openFile(argv[1]);
    if (get_nprocs > 0) {
        createThreads(file, 1, manageThreads)
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