/*********************************************************
 * Your Name: Parker Zimmerman
 * Partner Name: Yina Wang
 *********************************************************/
#include <stdio.h>
#include <stdlib.h>

long fileLength;

char* load_file(char const* path)
{
    char* buffer = 0;

    FILE * f = fopen (path, "rb"); //was "rb"

    if (f)
    {
      fseek (f, 0, SEEK_END);
      fileLength = ftell (f);
      fseek (f, 0, SEEK_SET);
      buffer = (char*)malloc ((fileLength+1)*sizeof(char));
      if (buffer)
      {
        if( 1!=fread( buffer , fileLength, 1 , f) )
            fclose(f),free(buffer),fputs("entire read fails",stderr),exit(1);
      }
      fclose (f);
    }
    buffer[fileLength] = '\0';
    for (int i = 0; i < fileLength; i++) {
        printf("buffer[%d] == %c\n", i, buffer[i]);
    }
    printf("buffer = %s\n", buffer);

    fwrite(buffer, sizeof(char), fileLength, stdout);
    return buffer;

    free(buffer);
}

int main(int argc, char *argv[]) {
    load_file(argv[1]);
}