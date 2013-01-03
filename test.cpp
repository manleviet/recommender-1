#include <iostream>
#include <stdio.h>
using namespace std;
int main(){
    FILE * fin;
    if((fin = fopen("datas/ratings.data", "wb")) == NULL) {
        printf("open error\n");
    }
    else fclose(fin);
    return 1;
}
