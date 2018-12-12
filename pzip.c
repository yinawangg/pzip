/*********************************************************
 * Your Name: Parker Zimmerman
 * Partner Name: Yina Wang
 *********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/mman.h>
#include <sys/sysinfo.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <unistd.h>


#include "thread.h"

#define BUFF_SIZE (4*1024)

// Data structure to hold both the character and its frequency
struct combo {
    int Num;
    char C;
};

// Data structure to hold the lock which will be used 
struct file {
    char * file; 
    struct combo *buffer; 
    int *combos;
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
    struct file *Fl;
};

// initializing the file conditions and locks
void pzip_init(struct file *fl){
    lock_init(&fl->f_lock);
    cond_init(&fl->finished_reading);
    cond_init(&fl->finished_writing);
    cond_init(&fl->finished_file);
}

// error checking for file opening
char *openFile(char const* path){
  int f = open(path,O_RDONLY);
  struct stat st;
  if (f){
      fstat(f,&st);
      char *fm = mmap(NULL,st.st_size,(PROT_READ|PROT_WRITE),MAP_SHARED,f,0); 
    return fm;
  } else{
    fputs("file open failed",stderr),exit(1);
  }
  close(f);
}

//reading the file in 4k chunks
void *read_file(void *arg) {
    struct thread_control *tc = (struct thread_control*)arg;
    struct file *fl = tc->Fl; 
    printf("tried to read");
    
    int t_num = tc->thread_num;
    int starting_pos = (t_num - 1)*(fl->chunk_sizes);
    int end_pos = (t_num * (fl->chunk_sizes)) - 1;
    struct combo current_combo;
    int numCombos = 0;
    current_combo.C = fl->file[starting_pos];
    current_combo.Num = 1;
    for (int n = starting_pos + 1; n < end_pos; n++){
          if (fl->file[n] == current_combo.C) {
              current_combo.Num++;
          } else{
              struct combo cmbo;
              cmbo.C = current_combo.C;
              cmbo.Num = current_combo.Num;
              fl->buffer[numCombos] = cmbo;
              numCombos++;
              current_combo.C = fl->file[n];
              current_combo.Num = 1;
          }
      }
      if (current_combo.Num > 1){
          fl->buffer[numCombos] = current_combo;
    }
    fl->combos[tc->thread_num] = numCombos;
    free(tc);   
    return NULL; 
}

// parallelized zipping 
void *pzip(void *arg) {
    struct thread_control *tc = (struct thread_control*)arg;
    struct file *fl = tc->Fl; 
    printf("tried to write");
    lock_acquire(&fl->f_lock);
    int t_num = tc->thread_num;
    int starting_pos = t_num*fl->chunk_sizes;
    int end_pos = starting_pos + fl->combos[tc->thread_num];

    for (int n = starting_pos; n < end_pos; n++){ 
        fwrite(&fl->buffer, sizeof(struct combo), 1, stdout);
    }
    lock_release(&fl->f_lock); 
    return NULL;          
}

void create_r_threads(struct file *fl, int num, pthread_t *thrd){
    printf("tried to make threads");
    for (int n = 1; n <= num; n++) {
        printf("1");
        struct thread_control *thrd_cntrl = (struct thread_control*)malloc(sizeof(struct thread_control));
        printf("2");
        thrd_cntrl->thread_num = n;
        printf("3");
        thrd_cntrl->Fl = fl;
        printf("4");
        pthread_t tid;
        printf("5");
        int ret = pthread_create(&tid, NULL, read_file, thrd_cntrl);
        //int set = pthread_setname_np(&tid,"%d%d",n-1,fun);
        printf("made thread %lu", tid);
        if (ret != 0) {
            perror("pthread_create");
            exit(1);
        }
        printf("   6");
        
        thrd[n-1] = tid;
        printf("7");
        
        printf("8");
    }
    printf("9");
}

void *manageThreads(void *arg) {
    printf("1\n");
    struct file *fl = (struct file*)arg;
    printf("1\n");
    fl->num_read_threads = 1; //num - 1;
    printf("1\n");
    fl->num_zip_threads = 1;
    printf("1\n");
    fl->chunk_sizes = BUFF_SIZE;
    printf("1\n");
    fl->buffer = (struct combo*)malloc(BUFF_SIZE*fl->num_read_threads*sizeof(struct combo));
    printf("1\n");
    fl->combos = (int*)malloc(fl->num_read_threads*sizeof(int));
    printf("1\n");
    pthread_t *rthrd = (pthread_t *)(malloc(fl->num_read_threads*sizeof(pthread_t)));
    printf("1\n");
    create_r_threads(fl, fl->num_read_threads, rthrd);
    printf("called createThreads");
    for (int n=0; n<fl->num_read_threads;n++){
        printf("joining %lu", rthrd[n]);
        pthread_join(rthrd[n], NULL);
        pthread_t tid;
        struct thread_control thrd_cntrl;
        thrd_cntrl.thread_num = n+1;
        thrd_cntrl.Fl = fl;
        int ret = pthread_create(&tid, NULL, pzip, &thrd_cntrl);
        if (ret != 0) {
            perror("pthread_create");
            exit(1);
        }
        if (n == fl->num_read_threads - 1){
            pthread_join(tid,NULL);
        }
    }
    // once all read threads have terminated the buffer has been filled
    // and we can write to stdout
    free(fl->buffer);
    free(fl->combos);
    free(rthrd);
    return NULL;
}

int main(int argc, char *argv[]){
    printf("1\n");
    struct file fl;
 	pzip_init(&fl);
    printf("1\n");
    fl.file = openFile(argv[1]);
    printf("1");
    if (get_nprocs > 0) {
       pthread_t tid;
       int ret = pthread_create(&tid, NULL, manageThreads, &fl);
       if (ret != 0) {
          perror("pthread_create");
          exit(1);
       }
       pthread_join(tid,NULL);
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