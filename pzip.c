/*********************************************************
 * Your Name: Parker Zimmerman
 * Partner Name: Yina Wang
 *********************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

// Data structure to hold both the character and its frequency
struct combo {
    int num;
    char c;
};

// Data structure to hold the lock which will be used 
struct fileInfo {
    struct lock lock;
    struct condition finished;    
};

int pzip() {
    
    return 0;
}

createThreads(int num){
  for (int n = 1; n <= num; n++) {
    pthread_t tid;
		int ret = pthread_create(&tid, NULL, passenger_thread, &station);
		if (ret != 0) {
			perror("pthread_create");
			exit(1);
  }
}

int main(int argc, char *argv[]) {
    if (readFile(argv[1]) == 0){
        pzipBuf();
    };

    //free(buffer);

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