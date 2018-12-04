/*********************************************************
 * Your Name: Parker Zimmerman
 * Partner Name: Yina Wang
 *********************************************************/


#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

long FileLength = 0;
char* buffer = 0;

int readFile(char const* path)
{

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
    int num = 0;
    bool haveNum = false;
    for (int i = 0; i <= FileLength; i++){
        if (buffer[i] == '\0' ){
            if (!haveNum) {
                return 0;
            } else {
                free(buffer),fputs("error in zipped file: failed",stderr),exit(1);
            }
        } else if (isdigit(buffer[i])){
            if (!haveNum) {
                num = buffer[i];
                haveNum = true;
            } else {
                free(buffer),fputs("error in zipped file: failed",stderr),exit(1);
            }
        } else if (isalpha(buffer[i])){
            if (haveNum){
                for (int n = 1; n <= num; n++){
                    char c = buffer[i];
                    fwrite(&c, sizeof(char), 1, stdout);
                }
                haveNum = false;
            } else {
                free(buffer),fputs("error in zipped file: failed",stderr),exit(1);
            }
        }
        else {
            free(buffer),fputs("error in zipped file: failed",stderr),exit(1);
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    if (readFile(argv[1]) == 0){
        zipBuf();
    };

    free(buffer);

    return 0;
}