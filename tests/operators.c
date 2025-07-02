#include<stdio.h>

int main(){
    int a;
    int b;
    a =8;
    b =2;
    printf(" %d",a+b);
    a++;
    b--;
    printf(" %d",a-b);
    --a;
    ++b;
    printf(" %d",a*b);
    printf(" %d",a/b);
    printf(" %c %d \n",'a',a%b);
    return 1;
}