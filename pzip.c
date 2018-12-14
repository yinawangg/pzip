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

// struct combo *copy_combo(struct combo cmbo){
// 	struct combo *r_combo;
// 	r_combo->C = cmbo.C;
// 	r_combo->Num = cmbo.Num;
// 	return r_combo;
// }

// Data structure to hold the lock which will be used 
struct file {
    char * file; 
    char *buffer; 
    int file_size;
    int *combos;
    char last_char;
    int last_num;
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
    fl->last_char = -1;
    fl->last_num = 0;
    lock_init(&fl->f_lock);
    cond_init(&fl->finished_reading);
    cond_init(&fl->finished_writing);
    cond_init(&fl->finished_file);
}

// error checking for file opening
void openFile(char const* path, struct file *file){
  int f = open(path,O_RDONLY);
  struct stat st;
  if (f){
      fstat(f,&st);
      file->file = mmap(NULL,st.st_size,PROT_READ,MAP_PRIVATE,f,0); 
      file->file_size = (int) st.st_size;
  } else{
    fputs("file open failed",stderr),exit(1);
  }
  close(f);
}

//reading the file in 4k chunks
void *read_file(void *arg) {

    struct thread_control *tc = (struct thread_control*)arg;
    struct file *fl = tc->Fl;

    int t_num = tc->thread_num;
    int starting_pos = (t_num - 1)*(fl->chunk_sizes);
    int end_pos = (t_num * (fl->chunk_sizes)) - 1;
    if (end_pos > fl->file_size) {
        end_pos = fl->file_size;
    }
    //struct combo current_combo;
    int combo_pos = 0;
    char C = fl->file[starting_pos];
    int Num = 1;
    int num_combos = 0;
    for (int n = starting_pos + 1; n < end_pos; n++){
          if (fl->file[n] == C) {
              Num++;
          } else{
              printf("--C-- = %c \n", C);
              printf("--N-- = %d \n", Num);
              fl->buffer[combo_pos] = Num;
              combo_pos = combo_pos + 4;
              fl->buffer[combo_pos] = C;
              combo_pos++;
              C = fl->file[n];
              Num = 1;
              num_combos++;
          }
      }
      if (Num > 1){
              printf("--C-- = %c \n", C);
              printf("--N-- = %d \n", Num);
              fl->buffer[combo_pos] = Num;
              combo_pos = combo_pos + 4;
              fl->buffer[combo_pos] = C;
              combo_pos++;
              num_combos++;
    }
    fl->combos[tc->thread_num-1] = num_combos;
    free(tc);   
    return NULL; 
}

// "parallelized" writing 
void *pzip(void *arg) {
    struct thread_control *tc = (struct thread_control*)arg;
    struct file *fl = tc->Fl; 
    lock_acquire(&fl->f_lock);
    int t_num = tc->thread_num;
    int starting_pos = (t_num-1)*fl->chunk_sizes;
    int starting_num = fl->buffer[starting_pos];
    //int starting_num = *(starting_num_pos);
    char starting_char = fl->buffer[starting_pos + 4];
    int end_pos = starting_pos + (fl->combos[tc->thread_num-1] * 5);
    if (end_pos > fl->file_size) {
        end_pos = fl->file_size;
    }
    if (starting_char == fl->last_char) {
        starting_num = starting_num + fl->last_num;
        fwrite(&starting_num, sizeof(int), 1, stdout);
        fwrite(&starting_char, sizeof(char), 1, stdout);
    }
    for (int n = starting_pos + 5; n < end_pos; n++){
        fwrite(&fl->buffer[n], sizeof(int), 1, stdout);
        n = n + 4;
        fwrite(&fl->buffer[n], sizeof(char), 1, stdout);
    }
    lock_release(&fl->f_lock); 
    return NULL;          
}

void create_r_threads(struct file *fl, int num, pthread_t *thrd){
    for (int n = 1; n <= num; n++) {
        struct thread_control *thrd_cntrl = (struct thread_control*)malloc(sizeof(struct thread_control));
        thrd_cntrl->thread_num = n;
        thrd_cntrl->Fl = fl;
        pthread_t tid;
        int ret = pthread_create(&tid, NULL, read_file, thrd_cntrl);
        if (ret != 0) {
            perror("pthread_create");
            exit(1);
        }
        thrd[n-1] = tid;
    }
}

void *manageThreads(void *arg) {
    struct file *fl = (struct file*)arg;
    fl->num_read_threads = 1; //num - 1;
    fl->num_zip_threads = 1;
    fl->chunk_sizes = BUFF_SIZE;
    fl->buffer = (char*)malloc(BUFF_SIZE*fl->num_read_threads*(sizeof(int)+sizeof(char)));
    fl->combos = (int*)malloc(fl->num_read_threads*sizeof(int));
    pthread_t *rthrd = (pthread_t *)(malloc(fl->num_read_threads*sizeof(pthread_t)));
    create_r_threads(fl, fl->num_read_threads, rthrd);




    for (int n=0; n<fl->num_read_threads;n++){
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
    free(fl->buffer);
    free(fl->combos);
    free(rthrd);
    return NULL;
}

int main(int argc, char *argv[]){
    struct file fl;
 	pzip_init(&fl);
    openFile(argv[1], &fl);
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
