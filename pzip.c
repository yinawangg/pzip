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
// global variable for buffer size
#define BUFF_SIZE (4*1024)

// Data structure to hold the file information as well as lock
// and conditions
struct file {
    char * file;
    // end of file boolean
    int eof; 
    char *buffer; 
    int file_size;
    int loop_num;
    int *combos;
    int *read_complete;
    int *has_end;
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
// Data structure to hold information about specific
// thread being run such as a thread number + file
struct thread_control {
    // serves as a thread ID except it goes from 0 to # of threads
    int thread_num;
    int reached_end;
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

// function for opening the file from the path given
void openFile(char const* path, struct file *file){
  int f = open(path,O_RDONLY);
  struct stat st;
  // if the file exists 
  if (f) {
      // use mmap to map the file to our file struct(??)
      fstat(f,&st);
      file->file = mmap(NULL,st.st_size,PROT_READ,MAP_PRIVATE,f,0); 
      file->file_size = (int) st.st_size;
      file->eof = 0;
  } else {
    // error checking for file opening
    fputs("file open failed",stderr),exit(1);
  }
  close(f);
}

// function to read the file
void *read_file(void *arg) {
    // struct to store info for the specifc thread
    struct thread_control *tc = (struct thread_control*)arg;
    // set our file struct to the file
    struct file *fl = tc->Fl; 
    // assigning t_num to the thread id 
    int t_num = tc->thread_num;


    //printf("=========================reading with thread %d==================================\n", t_num);


    // the starting position in the file found by multiplying how many groups of threads running by 
    // the number of read threads and then adding thread id...and then mutiplying by the file's chunk size
    // that we decide to split it by
    int starting_pos = ((fl->loop_num * fl->num_read_threads) + t_num)*fl->chunk_sizes;
    
    
    //printf("\n\n\n\n\n%d\n\n\n\n\n", starting_pos);
    
    
    // the end position is found by add a chunk size to the starting position
    int end_pos = (starting_pos + (fl->chunk_sizes));
    // if the end position is past the file size
    if (end_pos >= fl->file_size) {
        // change end position to the last char of the file
        end_pos = (fl->file_size) - 1;
        // change end of file boolean to true
        //fl->eof = 1;
        fl->has_end[t_num] = 1;
    }
    // the position into the buffer of the next combo (group of letters)
    int combo_pos = t_num*(fl->chunk_sizes*(sizeof(int)+sizeof(char)));
    // variable to store the character being read
    char C;
    // if the starting position is before or at the ending position
    if (starting_pos <= end_pos){
        // then the character being read is the character at the starting
        // position of the file
        C = fl->file[starting_pos];
    } else {
        // if not, set the character to -1
        C = -1;

    }
    // number of times we have seen C
    int Num = 1;
    // number of letter groups we have seen
    int num_combos = 0;
    // if we are at the end of file
    if (fl->eof == 1) {
        // set Num back to 0
        Num = 0;
    }
    // iterate through all of the character in file buffer
    // from starting position to ending position
    for (int n = starting_pos + 1; n < end_pos; n++) {
        // if the character is equal to C
        // increment Num
        if (fl->file[n] == C) {
            Num++;
        // otherwise add the num and char to combo buffer
        // incrementing the current combo_pos    
        } else {
            
            
            // printf("buffer pos = %d\n", combo_pos);
            // printf("num -- %d\n", Num);
            // printf("char - %c\n", C);
            
            
            fl->buffer[combo_pos] = Num;
            
            
            // printf("num saved as -- %d\n", fl->buffer[combo_pos]);
            
            
            combo_pos = combo_pos + 4;
            fl->buffer[combo_pos] = C;
            
            
            // printf("char saved as -- %c\n\n", fl->buffer[combo_pos]);
            
            
            combo_pos++;
            C = fl->file[n];
            Num = 1;
            num_combos++;
        }
    }
    // if the Num is greater than 0
    // put Num into the file buffer at the combo's position
    // increat the combo position by four (size of int)
    // then put the character in the combo position
    // increment both combo position and the number of combos
    if (Num > 0){
        
        
        // printf("buffer pos = %d", combo_pos);
        // printf("num -- %d\n", Num);
        // printf("char - %c\n\n", C);
        
        
        fl->buffer[combo_pos] = Num;
        combo_pos = combo_pos + 4;
        fl->buffer[combo_pos] = C;
        combo_pos++;
        num_combos++;

    }
    // put the number of combinations read by current thread into the combo file
    fl->combos[t_num] = num_combos;
    // grab the lock
    lock_acquire(&fl->f_lock); 
    
    
    // printf("@@@@@@@@@@@@@@@@@@@@@@@@ finish thread %d @@@@@@@@@@@@@@@@@@@@@\n", t_num);
    
    
    fl->read_complete[t_num] = 1;
    // signal that we are finished with reading
    cond_signal(&fl->finished_reading, &fl->f_lock);
    // release the lock
    lock_release(&fl->f_lock);
    // free up the memory used for specific thread info
    free(tc);   
    return NULL; 
}

// function for "parallelized" writing 
void *pzip(void *arg) {
    
    
    // printf("printing");
    
    // grab our file
    struct file *fl = (struct file*)arg;
    // grab the lock
    lock_acquire(&fl->f_lock);
    // see if we reaching end of file
    int have_end = 0;
    // iterate through the number of read threads
    // and while the file has not been read, wait for 
    // that condition
    for (int t_num = 0; t_num < fl->num_read_threads; t_num++){
        
        
        // printf("=========================trying to write with thread %d==================================\n", t_num);
        
        
        while (fl->read_complete[t_num] == 0){
            cond_wait(&fl->finished_reading, &fl->f_lock);
        }
        
        
        // printf("=========================writing with thread %d==================================\n", t_num);
        
        // if it is not the end of the file
        if (have_end != 1){
            // and if this thread is the one that reached the end
            if (fl->has_end[t_num] == 1) {
                // set have_end to true
                have_end = 1;
            }
            // set the starting position to the thread id * file chunk * (size of int + size of char)
            int starting_pos =  t_num*(fl->chunk_sizes*(sizeof(int)+sizeof(char)));
            // end position to starting position to 
            int end_pos = starting_pos + ((fl->combos[t_num] - 1) * 5);
            int starting_num = fl->buffer[starting_pos];
            char starting_char = fl->buffer[starting_pos + 4];

        
            // printf("--start-- = %d \n", starting_pos);
            // printf("--end-- = %d \n", end_pos);

        
            if (starting_char == fl->last_char) {
                starting_num = starting_num + fl->last_num;
        
        
                // printf("starting num : %d \n", starting_num);
                // printf("starting char : %d \n\n", starting_char);
        
        
                fwrite(&starting_num, sizeof(int), 1, stdout);
                fwrite(&starting_char, sizeof(char), 1, stdout);
            } else {
        
        
                // printf("last num : %d \n", fl->last_num);
                // printf("last char : %d \n\n", fl->last_char);
	            // printf("starting num : %d \n", starting_num);
                // printf("starting char : %d \n\n", starting_char);
        
        
                fwrite(&fl->last_num, sizeof(int), 1, stdout);
                fwrite(&fl->last_char, sizeof(char), 1, stdout);
	            fwrite(&starting_num, sizeof(int), 1, stdout);
                fwrite(&starting_char, sizeof(char), 1, stdout);
            }
        
        
            // printf("starting with combo at %d and going to %d\n\n", starting_pos + 5, end_pos);
            // printf("starting with combo int = %d and char = %d\n\n", fl->buffer[starting_pos + 5], fl->buffer[starting_pos+9]);


            for (int n = starting_pos + 5; n < end_pos; n++){
                fwrite(&fl->buffer[n], sizeof(int), 1, stdout);
                n = n + 4;
                fwrite(&fl->buffer[n], sizeof(char), 1, stdout);
            }

            if (fl->has_end[t_num] == 1) {


                // printf("\n\n\n\n\n eof");


                // char *eof_line = "\n";
                fwrite(&fl->buffer[end_pos], sizeof(int), 1, stdout);
                fwrite(&fl->buffer[end_pos + 4], sizeof(char), 1, stdout);
                // fwrite(eof_line, sizeof(char), 1, stdout);
            } else {


                // printf("updating last char\n\n\n\n");


                fl->last_num = fl->buffer[end_pos];
                fl->last_char = fl->buffer[end_pos+4];
        
            }
        }
    }
    lock_release(&fl->f_lock);
    return NULL;          
}

void create_r_threads(struct file *fl, int num, pthread_t *thrd){
    for (int n = 0; n < num; n++) {
        struct thread_control *thrd_cntrl = (struct thread_control*)malloc(sizeof(struct thread_control));
        thrd_cntrl->thread_num = n;
        thrd_cntrl->Fl = fl;
        pthread_t tid;


        // printf("!!!!!!!!!!!!!!!!thread %d!!!!!!!!!!!!!!!!!\n", n);


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
    fl->num_read_threads = get_nprocs();
    fl->num_zip_threads = 1;
    fl->chunk_sizes = BUFF_SIZE;
    fl->buffer = (char*)malloc(BUFF_SIZE*fl->num_read_threads*(sizeof(int)+sizeof(char)));
    fl->combos = (int*)malloc(fl->num_read_threads*sizeof(int));
    pthread_t *rthrd = (pthread_t *)(malloc(fl->num_read_threads*sizeof(pthread_t)));
    fl->read_complete = (int *)(calloc(fl->num_read_threads,sizeof(int)));
    fl->has_end = (int *)(calloc(fl->num_read_threads,sizeof(int)));
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


        pthread_t tid;


        // printf("!!!!!!!!!!!!!!!!writing thread!!!!!!!!!!!!!!!!!\n");


        int ret = pthread_create(&tid, NULL, pzip, fl);
        if (ret != 0) {
            perror("pthread_create");
            exit(1);
        }
        pthread_join(tid,NULL);


        // int n = 0;
        // printf("-------------------------------------------------------------------------%d\n", n);
        // for (n; n < fl->num_read_threads; n++){
        //     printf("---------------------------------------------------------------------%d\n", n);
        //     lock_acquire(&fl->f_lock); 
        //     struct thread_control thrd_cntrl;
        //     printf("printing thread %d\n", n);
        //     thrd_cntrl.thread_num = n;
        //     thrd_cntrl.Fl = fl;
        //     printf("looking for thread %d\n", n);
        //     while(fl->read_complete[n] != 1){
        //         printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@need thread %d\n", n);
        //         cond_wait(&fl->finished_reading, &fl->f_lock);
        //         printf("@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@@got signal from thread %d\n", n);
        //     }
        //     pthread_t tid;
            
        //     printf("printing thread %d\n", thrd_cntrl.thread_num);
        //     int ret = pthread_create(&tid, NULL, pzip, &thrd_cntrl);
        //     if (ret != 0) {
        //         perror("pthread_create");
        //         exit(1);
        //     }
        //     if (n == fl->num_read_threads - 1){
        //         pthread_join(tid,NULL);
        //     }

        //     lock_release(&fl->f_lock);
        // }
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
