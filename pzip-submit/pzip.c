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
    int chunk_sizes;
    struct lock f_lock;
    struct condition finished_reading;
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
    cond_init(&fl->finished_reading);
}

// function for opening the file from the path given
void openFile(char const* path, struct file *file){
  int f = open(path,O_RDONLY);
  struct stat st;
  // if the file exists
  if (f) {
      // use mmap to map the file to our file struct
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

    // the starting position in the file found by multiplying how many groups of threads running by
    // the number of read threads and then adding thread id...and then mutiplying by the file's chunk size
    // that we decide to split it by
    int starting_pos = ((fl->loop_num * fl->num_read_threads) + t_num)*fl->chunk_sizes;

    // the end position is found by add a chunk size to the starting position
    int end_pos = (starting_pos + (fl->chunk_sizes));
    // if the end position is past the file size
    if (end_pos > fl->file_size) {
        // change end position to the last char of the file
        end_pos = (fl->file_size) - 1;
        // change end of file boolean to true
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

            fl->buffer[combo_pos] = Num;

            combo_pos = combo_pos + 4;
            fl->buffer[combo_pos] = C;

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

    fl->read_complete[t_num] = 1;
    // signal that we are finished with reading
    cond_signal(&fl->finished_reading, &fl->f_lock);
    // free up the memory used for specific thread info
    free(tc);
    // release the lock
    lock_release(&fl->f_lock);
    
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

        while (fl->read_complete[t_num] == 0){
            cond_wait(&fl->finished_reading, &fl->f_lock);
        }

        // if it is not the end of the file
        if (have_end != 1){
            // and if this thread is the one that reached the end
            if (fl->has_end[t_num] == 1) {
                // set have_end to true
                have_end = 1;
            }
            // set the starting position to the thread id * file chunk *
            // (size of int + size of char)
            int starting_pos =  t_num*(fl->chunk_sizes*
            (sizeof(int)+sizeof(char)));
            // end position from starting position added to combo buffer at
            // thread is times (size of int + size of char)
            int end_pos = starting_pos + ((fl->combos[t_num]) * 5);
            // get the starting number
            int starting_num = fl->buffer[starting_pos];
            // and the starting character
            char starting_char = fl->buffer[starting_pos + 4];
            // if the starting character is the file's last char
            if (starting_char == fl->last_char /*|| (fl->loop_num == 0 && t_num == 0)*/ ) {
                // then the last number in file is added to the starting num
                starting_num = starting_num + fl->last_num;
                // write the int and char for this combo to the output file
                fwrite(&starting_num, sizeof(int), 1, stdout);
                fwrite(&starting_char, sizeof(char), 1, stdout);
                // otherwise, write the files last number and character to output
                // and then write the starting num and char to output
            } else {
                if( !(fl->loop_num == 0 && t_num == 0)){
                fwrite(&fl->last_num, sizeof(int), 1, stdout);
                fwrite(&fl->last_char, sizeof(char), 1, stdout);
                }
                if(fl->combos[t_num] > 1 || (fl->has_end[t_num] == 1 && fl->combos[t_num] > 1)){
	                fwrite(&starting_num, sizeof(int), 1, stdout);
                    fwrite(&starting_char, sizeof(char), 1, stdout);
                }
            }

            // iterate through the buffer and start from the second combos
            // write the number of letters to the output file
            // and then write the actual character
            for (int n = starting_pos + 5; n < end_pos-5; n++){
                fwrite(&fl->buffer[n], sizeof(int), 1, stdout);
                n = n + 4;
                fwrite(&fl->buffer[n], sizeof(char), 1, stdout);
            }
            // if this thread has the end of the file
            // then write the number and character to the output file
            if (fl->has_end[t_num] == 1) {
                // else, set the last num and char to what is at the end of the
                // buffer
            } else {


                fl->last_num = fl->buffer[end_pos-5];
                fl->last_char = fl->buffer[end_pos-5+4];

            }
        }
    }
    // release the lock
    lock_release(&fl->f_lock);
    return NULL;
}

// function for creating read threads
void create_r_threads(struct file *fl, int num, pthread_t *thrd){
    // iterate through the number of threads needed to be made
    // every thread will have its own control thread
    // that has its thread id and the file
    // and it will create a thread with a real pthread id with our read file
    // and the control info thread
    for (int n = 0; n < num; n++) {
        struct thread_control *thrd_cntrl = (struct thread_control*)malloc(sizeof(struct thread_control));
        thrd_cntrl->thread_num = n;
        thrd_cntrl->Fl = fl;
        pthread_t tid;

        int ret = pthread_create(&tid, NULL, read_file, thrd_cntrl);
        // error checking for pthread_create
        if (ret != 0) {
            perror("pthread_create");
            exit(1);
        }
        // set the thread at n to be the real pthread id
        thrd[n] = tid;
    }
}

// function that serves as our task manager thread
void *manageThreads(void *arg) {
    struct file *fl = (struct file*)arg;
    //int num = get_nprocs();
    // number of read threads will be the number of available threads
    fl->num_read_threads = get_nprocs();
    // the chunk size will be the buffer size
    fl->chunk_sizes = BUFF_SIZE;
    // allocating memory for the file buffer
    fl->buffer = (char*)malloc(BUFF_SIZE*fl->num_read_threads*
      (sizeof(int)+sizeof(char)));
    // allocating memory for the combo buffer
    fl->combos = (int*)malloc(fl->num_read_threads*sizeof(int));
    // allocate memory for all the read threads
    pthread_t *rthrd = (pthread_t *)
      (malloc(fl->num_read_threads*sizeof(pthread_t)));
    // allocate memory for the read_complete buffer
    fl->read_complete = (int *)(calloc(fl->num_read_threads,sizeof(int)));
    // allocate memory for the has end buffer
    fl->has_end = (int *)(calloc(fl->num_read_threads,sizeof(int)));
    // one loop size is the number of threads * the chunk size
    int loop_size = fl->num_read_threads * fl->chunk_sizes;
    // divide the file size by the loop size
    div_t loops_d = div(fl->file_size,loop_size);
    int loops = 0;
    // if there are remainders, add one to the quotient of loops_d
    if (loops_d.rem != 0) {
	    loops = loops_d.quot + 1;
    } else {
      // if not, then just keep the answer
	    loops = loops_d.quot;
    }
    // for every loop, set the loop number to the index
    // and create the read threads
    for (int l = 0; l < loops; l++){
        fl->loop_num = l;
        create_r_threads(fl, fl->num_read_threads, rthrd);

        //create a writing thread
        pthread_t tid;
        int ret = pthread_create(&tid, NULL, pzip, fl);
        // error checking for this pthread_create
        if (ret != 0) {
            perror("pthread_create");
            exit(1);
        }
        pthread_join(tid,NULL);

    }
    // free the file and combo buffer
    free(fl->has_end);
    free(fl->read_complete);
    free(fl->buffer);
    free(fl->combos);
    free(rthrd);
    return NULL;
}

// main function where we create+ initiate our file struct and open the file
int main(int argc, char *argv[]){
    struct file fl;
 	pzip_init(&fl);
    openFile(argv[1], &fl);
    // if there are threads available
    if (get_nprocs > 0) {
      // create a manager thread
       pthread_t tid;
       int ret = pthread_create(&tid, NULL, manageThreads, &fl);
       // error checking
       if (ret != 0) {
          perror("pthread_create");
          exit(1);
       }
       pthread_join(tid,NULL);
    }
     return 0;
}
