#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <unistd.h> 
#define int int64_t
#define POOL_SIZE 256*1024

int token;
char* src;
char* prev_src;
int pool_size;
int line_num;

void next_token(){
    token = *(src++);
}

void expression(int level) {
    // do nothing
}
void program(){
    next_token();
    while(token >0){
        printf("token = %c\n",token);
        next_token();
    }
}
int eval(){
    return 0;
}

int main(int argc, char** argv) {
    argc--;
    argv++;
   // int fd;
    int i;
    pool_size = POOL_SIZE;
    line_num = 1;
    FILE* fp = fopen(*argv,"rb");
    if(!fp){
        printf("ERROR: coud not open the file (%s)\n",*argv);
        return -1;
    }
    src = malloc(pool_size);
    if(!src){
        printf("ERROR: coud not allocet size of %d\n",pool_size);
        fclose(fp);
        return -1;
    }
    prev_src =src;
    
     i = fread(src, sizeof(char), pool_size - 1, fp);
     if (ferror(fp)) {
        perror("ERROR: at fread() function");
        free(src); 
        fclose(fp);
        return -1;
    }
     src[i]=0;
     fclose(fp);

     program();
     free(src);
     return eval();
}