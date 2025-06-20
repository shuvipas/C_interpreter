#include <stdio.h>
#include <stdlib.h>
typedef struct dict{
    int key;
    int value;
}dict;
int dict_cmp(const void* a,const void*b){
    int aval = ((dict*)a)->value;
    return ((dict*)a)->value -((dict*)b)->value;
}
void dict_sort(struct dict* arr,int arrSize){
    qsort(arr,arrSize, sizeof(dict),dict_cmp);
}

int main() {
    printf("Hello, C!\n");
    int x = 1;

    char *y = (char*)&x;
  
    printf("%c\n",*y+48); // 1 = little endian , 0 = big endian
    return 0;
}