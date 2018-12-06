/*********************************************************
 * Your Name: Parker Zimmerman
 * Partner Name: Yina Wang
 *********************************************************/


#include <stdio.h>
#include <stdlib.h>

#define BUFF_SIZE (4*1024)

FILE *openFile(char const* path){
  FILE* f =fopen(path,"rb");
  if (f){
    return f;
  } else{
    fputs("file open failed",stderr),exit(1);
  }
}


int zipFile(FILE * f){
  char *buffer = (char*)malloc (BUFF_SIZE*sizeof(char));
  int n = 0;
  char lastChar = -1;
  int numOccur = 0;
  if (buffer){
    while((n = fread(buffer, sizeof(char), BUFF_SIZE, f))){
      if (!numOccur) {
        numOccur = 1;
        lastChar = buffer[0];
      }
      for (int c = 1; c <= n; c ++){
          if (buffer[c] == lastChar) {
              numOccur++;
          } else{
              fwrite(&numOccur, sizeof(int), 1, stdout);
              fwrite(&lastChar, sizeof(char), 1, stdout);
              numOccur = 1;
          }
          lastChar = buffer[c];
      }
    }
    if (numOccur > 1){
      fwrite(&numOccur, sizeof(int), 1, stdout);
      fwrite(&lastChar, sizeof(char), 1, stdout);
    }
  }
    free(buffer);
    return 0;
}

int main(int argc, char *argv[]) {
  FILE *file = openFile(argv[1]);
  zipFile(file);
  fclose(file);

    return 0;
}


    //fwrite(buffer, sizeof(char), FileLength, stdout);
