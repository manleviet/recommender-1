#include <iostream>
#include <stdio.h>
#include <string.h>
#include <list>
#include <unistd.h>
#include <algorithm>
using namespace std;
#define QUERYNUM 2817124//问题数量
#define USERNUM 480189
int UserQueryIdx[USERNUM+1];
int UserQuery[QUERYNUM];
FILE *answerFile=fopen("predict.txt","w");
int main(){
    memset(UserQueryIdx,0,sizeof(UserQueryIdx));//初始个数-0
    FILE *queryFile=fopen("datas/fmt_query.txt","r");
    if(queryFile==NULL) {
        printf("please run ./data2bin to generate fmt_query.txt\n");
        return 1;
    }
    char line[100]; char date[30]; int userid;int movieid;int querynum=0;
    while(fscanf(queryFile, "%s", line) != EOF) {
        if (line[strlen(line) - 1] == ':') {
            movieid = atoi(line);
        } else {
            sscanf(line, "%d,%s", &userid, date);
            querynum++;
            UserQueryIdx[userid]++;
        }
    }
    fclose(queryFile);

    for(int i=2;i<=USERNUM;i++)
        UserQueryIdx[i]+=UserQueryIdx[i-1];
    printf("total %d questions \n",querynum);
    FILE *userQueryIdxFile=fopen("datas/user_queryidx.data","wb");
    fwrite(UserQueryIdx,sizeof(int),USERNUM+1,userQueryIdxFile);
    fclose(userQueryIdxFile);
    

    return 0;
}   

