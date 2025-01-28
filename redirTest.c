#include <stdio.h>

int main(){
    char s[10];
    fgets(s,10,stdin);
    printf("%s\n",s);
    return 0;
}