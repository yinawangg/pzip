/*********************************************************
 * Your Name: Parker Zimmerman
 * Partner Name: Yina Wang
 *********************************************************/
/*#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>

long FileLength = 0;
char* buffer = 0;

int readFile(char const* path){
  // divide file into chunks for each thread to read
  // need to have way to reference each thread and assign
  // it a chunk and section of buffer to write to that way
  // there won't be overlap
    FILE * f = fopen (path, "rb");

    if (f) {
      fseek (f, 0, SEEK_END);
      FileLength = ftell (f);
      fseek (f, 0, SEEK_SET);
      buffer = (char*)malloc ((FileLength+1)*sizeof(char));
      if (buffer){
        if( 1!=fread( buffer , FileLength, 1 , f) )
            fclose(f),free(buffer),fputs("entire read fails",stderr),exit(1);
      }
      fclose (f);
    }
    buffer[FileLength] = '\0';

    return 0;
}

int zipBuf(){
  //
    char lastChar = buffer[0];
    int numOccur = 1;
    for (int c = 1; c <= FileLength; c ++){
        if (buffer[c] == lastChar) {
            numOccur++;
        } else {
            fwrite(&numOccur, sizeof(int), 1, stdout);
            fwrite(&lastChar, sizeof(char), 1, stdout);
            numOccur = 1;

        }
        lastChar = buffer[c];


    }
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
*/
int main(int argc, char *argv[]) {
//    createThreads(get_nprocs());
//    if (readFile(argv[1]) == 0){
//        zipBuf();
//    };
//
//    free(buffer);
//
    return 0;
}
