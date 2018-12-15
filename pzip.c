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

// Data structure to hold the lock which will be used 
struct file {
    char * file;
    int eof; 
    char *buffer; 
    int file_size;
    int loop_num;
    int *combos;
    int *read_complete;
    char last_char;
    int last_num;
    int num_read_threads;
    int num_zip_threads;
    int chunk_sizes;
    struct lock f_lock;
    struct condition made_threads;
    struct condition finished_reading;
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
    cond_init(&fl->made_threads);
    cond_init(&fl->finished_reading);
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
      file->eof = 0;
  } else{
    fputs("file open failed",stderr),exit(1);
  }
  close(f);
}

//reading the file in 4k chunks
void *read_file(void *arg) {
    struct thread_control *tc = (struct thread_control*)arg;
    struct file *fl = tc->Fl;
    //lock_acquire(&fl->f_lock); 
    int t_num = tc->thread_num;
    printf("=========================reading with thread %d==================================\n", t_num);
    int starting_pos = ((fl->loop_num * fl->num_read_threads) + t_num)*fl->chunk_sizes;
    printf("\n\n\n\n\n%d\n\n\n\n\n", starting_pos);
    int end_pos = (starting_pos + (fl->chunk_sizes));
    if (end_pos >= fl->file_size) {
        end_pos = (fl->file_size) - 1;
        fl->eof = 1;
    }
    int combo_pos = t_num*(fl->chunk_sizes*(sizeof(int)+sizeof(char)));
    char C;
    if (starting_pos <= end_pos){
        C = fl->file[starting_pos];
    } else{
        C = -1;
    }
    int Num = 1;
    int num_combos = 0;
    if (fl->eof == 1) {
        Num = 0;
    }
    for (int n = starting_pos + 1; n < end_pos; n++) {
        
        if (fl->file[n] == C) {
            Num++;

        } else {
            printf("buffer pos = %d\n", combo_pos);
            printf("num -- %d\n", Num);
            printf("char - %c\n", C);
            fl->buffer[combo_pos] = Num;
            printf("num saved as -- %d\n", fl->buffer[combo_pos]);
            combo_pos = combo_pos + 4;
            fl->buffer[combo_pos] = C;
            printf("char saved as -- %c\n\n", fl->buffer[combo_pos]);
            combo_pos++;
            C = fl->file[n];
            Num = 1;
            num_combos++;
        }
    }
    if (Num > 0){
        printf("buffer pos = %d", combo_pos);
        printf("num -- %d\n", Num);
        printf("char - %c\n\n", C);
        fl->buffer[combo_pos] = Num;
        combo_pos = combo_pos + 4;
        fl->buffer[combo_pos] = C;
        combo_pos++;
        num_combos++;

    }
    fl->combos[t_num] = num_combos;
    lock_acquire(&fl->f_lock); 
    printf("@@@@@@@@@@@@@@@@@@@@@@@@ finish thread %d @@@@@@@@@@@@@@@@@@@@@\n", t_num);
    fl->read_complete[t_num] = 1;
    cond_signal(&fl->finished_reading, &fl->f_lock);
    lock_release(&fl->f_lock);
    free(tc);   
    return NULL; 
}

// "parallelized" writing 
void *pzip(void *arg) {
    printf("printing");
    struct thread_control *tc = (struct thread_control*)arg;
    struct file *fl = tc->Fl; 
    //lock_acquire(&fl->f_lock);
    int t_num = tc->thread_num;
    printf("=========================writing with thread %d==================================\n", t_num);
    int starting_pos =  t_num*(fl->chunk_sizes*(sizeof(int)+sizeof(char)));
    int starting_num = fl->buffer[starting_pos];
    char starting_char = fl->buffer[starting_pos + 4];
    int end_pos = starting_pos + ((fl->combos[t_num] - 1) * 5);

    printf("--start-- = %d \n", starting_pos);
    printf("--end-- = %d \n", end_pos);

    if (starting_char == fl->last_char) {
        starting_num = starting_num + fl->last_num;
        printf("starting num : %d \n", starting_num);
        printf("starting char : %d \n\n", starting_char);
        // fwrite(&starting_num, sizeof(int), 1, stdout);
        // fwrite(&starting_char, sizeof(char), 1, stdout);
    } else {
        printf("last num : %d \n", fl->last_num);
        printf("last char : %d \n\n", fl->last_char);
	    printf("starting num : %d \n", starting_num);
        printf("starting char : %d \n\n", starting_char);
        // fwrite(&fl->last_num, sizeof(int), 1, stdout);
        // fwrite(&fl->last_char, sizeof(char), 1, stdout);
	    // fwrite(&starting_num, sizeof(int), 1, stdout);
        // fwrite(&starting_char, sizeof(char), 1, stdout);
    }
    printf("starting with combo at %d and going to %d\n\n", starting_pos + 5, end_pos);
    printf("starting with combo int = %d and char = %d\n\n", fl->buffer[starting_pos + 5], fl->buffer[starting_pos+9]);
    for (int n = starting_pos + 5; n < end_pos; n++){
        // fwrite(&fl->buffer[n], sizeof(int), 1, stdout);
        // n = n + 4;
        // fwrite(&fl->buffer[n], sizeof(char), 1, stdout);
    }
    if (fl->eof == 1) {
        printf("\n\n\n\n\n eof");
        // char *eof_line = "\n";
        // fwrite(&fl->buffer[end_pos], sizeof(int), 1, stdout);
        // fwrite(&fl->buffer[end_pos + 4], sizeof(char), 1, stdout);
        // fwrite(eof_line, sizeof(char), 1, stdout);
    } else {
        printf("updating last char\n\n\n\n");
        fl->last_num = fl->buffer[end_pos];
        fl->last_char = fl->buffer[end_pos+4];
    }
    //lock_release(&fl->f_lock); 
    return NULL;          
}

void create_r_threads(struct file *fl, int num, pthread_t *thrd){
    for (int n = 0; n < num; n++) {
        struct thread_control *thrd_cntrl = (struct thread_control*)malloc(sizeof(struct thread_control));
        thrd_cntrl->thread_num = n;
        thrd_cntrl->Fl = fl;
        pthread_t tid;
        printf("!!!!!!!!!!!!!!!!thread %d!!!!!!!!!!!!!!!!!\n", n);
        int ret = pthread_create(&tid, NULL, read_file, thrd_cntrl);
        if (ret != 0) {
            perror("pthread_create");
            exit(1);
        }
        thrd[n] = tid;
    }
}

void *manageThreads(void *arg) {
    struct file *fl = (struct file*)arg;
    //int num = get_nprocs();
    fl->num_read_threads = 2;
    fl->num_zip_threads = 1;
    fl->chunk_sizes = BUFF_SIZE;
    fl->buffer = (char*)malloc(BUFF_SIZE*fl->num_read_threads*(sizeof(int)+sizeof(char)));
    fl->combos = (int*)malloc(fl->num_read_threads*sizeof(int));
    pthread_t *rthrd = (pthread_t *)(malloc(fl->num_read_threads*sizeof(pthread_t)));
    fl->read_complete = (int *)(calloc(fl->num_read_threads,sizeof(int)));
    int loop_size = fl->num_read_threads * fl->chunk_sizes;
    div_t loops_d = div(fl->file_size,loop_size);
    int loops = 0;
    if (loops_d.rem != 0) {
	    loops = loops_d.quot + 1;
    } else {
	    loops = loops_d.quot;
    }
    for (int l = 0; l < loops; l++){
        fl->loop_num = l;
        create_r_threads(fl, fl->num_read_threads, rthrd);
        int n = 0;
        printf("-------------------------------------------------------------------------%d\n", n);
        for (n; n < fl->num_read_threads; n++){
            printf("---------------------------------------------------------------------%d\n", n);
            lock_acquire(&fl->f_lock); 
            struct thread_control thrd_cntrl;
            printf("printing thread %d\n", n);
            thrd_cntrl.thread_num = n;
            thrd_cntrl.Fl = fl;
            printf("looking for thread %d\n", n);
            while(fl->read_complete[n] != 1){
                printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@need thread %d\n", n);
                cond_wait(&fl->finished_reading, &fl->f_lock);
                printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@got signal from thread %d\n", n);
            }
            pthread_t tid;
            
            printf("printing thread %d\n", thrd_cntrl.thread_num);
            int ret = pthread_create(&tid, NULL, pzip, &thrd_cntrl);
            if (ret != 0) {
                perror("pthread_create");
                exit(1);
            }
            if (n == fl->num_read_threads - 1){
                pthread_join(tid,NULL);
            }

            lock_release(&fl->f_lock);
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
