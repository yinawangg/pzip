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


int unzipFile(FILE * f){
  char currChar = -1;
  int numOccur = 0;
  while((fread(&numOccur, sizeof(int), 1, f) &&
          fread(&currChar, sizeof(char), 1, f))){
    for (int n = 1; n <= numOccur; n++){
      fwrite(&currChar, sizeof(char), 1, stdout);
    }
  }
  return 0;
}

int main(int argc, char *argv[]) {
  FILE *file = openFile(argv[1]);
  unzipFile(file);
  fclose(file);

  return 0;
}
