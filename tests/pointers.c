#include<stdio.h>

int main(){
    int* a;
    int** b;
    int num;
    char c;
    c ='s';
    num =1;
    a = &num;
    b= &a;
    *a =8;
    **b =2;
    printf("%c %d %d %d",c, num, *a, **b);
    

    return 1;
}