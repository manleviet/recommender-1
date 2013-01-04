#include <fstream>
#include <algorithm>
#include <math.h>
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <map>
#include "handledata.h"
using namespace std;
#define cnt_rating 100480507
//从预处理后的文件rating.data读入的数据：

struct Rating {
    int movieid, userid, score, next;
    void init(int _movieid, int _userid, int _score) {
        movieid = _movieid, userid = _userid, score = _score;
    }
//    score 按照用户id和评分排序
//    bool operator <(const Rating &a) const {
//        return userid < a.userid || userid == a.userid && score > a.score;
//    }
}Ratings[cnt_rating];
//注意：movie和user从1开始编号
struct Movie {
    int head, end, cnt;//Ratings起始下标head和电影评分个数cnt
    double score, dat_score;//总评分
    double sigma;
    Movie () {
        //初始化
        head = -1, cnt = 0, score = 0, dat_score = 0; sigma = 0 ;
    }
}movies[MOVIENUM + 1];
struct User {
    int head, cnt;//Ratings下标和电影评分个数
    double score, dat_score;
    double avgscore;
    double sigma;
    User () {
        //初始化
        head = -1, cnt = 0, score = 0, dat_score = 0;sigma = 0 ;
    }
}users[USERNUM + 1];
void initdata(){
    FILE *ratingDataFile=fopen("datas/rating.data","r");
    fread(Ratings, sizeof(Rating), cnt_rating, ratingDataFile);
    fclose(ratingDataFile);
    //读入预处理过后的数据
    for(int i =cnt_rating-1; i>=0; --i) {
        int movieid = Ratings[i].movieid;
        int userid = Ratings[i].userid;
        int score = Ratings[i].score;
        movies[movieid].head = i;
        movies[movieid].score += score;
        ++ movies[movieid].cnt;

        users[userid].head = i;
        users[userid].score += score;
        ++ users[userid].cnt;//用户评分的电影个数
    }
//    //测试
//    for(int i=1;i<=100;i++){
//        printf("\nuser:%d\n",i);
//        int head=users[i].head;
//        while(head>=0)
//            printf("%d ",head),head=Ratings[head].next;
//    }
    
    //计算用户平均分、斯格玛
    for(int i=1;i<=USERNUM;i++){
        printf("\nuser:%d\n",i);
        users[i].avgscore=users[i].cnt==0?0:(double)users[i].score/users[i].cnt;
        int head=users[i].head;
        while(head>=0){
           // printf("%d ",head)D,DD
           // printf("score:%d avl:%f\n",Ratings[head].score,users[i].avgscore);
            users[i].sigma+=pow(Ratings[head].score-users[i].avgscore,2);
            head=Ratings[head].next;
        }
        users[i].sigma=sqrt(users[i].sigma);
        if(i<1000) printf("avl:%f sigma:%f\n",users[i].avgscore,users[i].sigma);
    }
}


//计算用户i和其他人的相似度
map<int,float> getUserSim(int l){

    map<int , float> sims;
    int i=users[l].head;
    while(i>0)
    {
        Rating rate=Ratings[i];int score=rate.score;int movieid=rate.movieid;
        float x=(float)score-users[l].avgscore;
        int j=movies[movieid].head;
        while(Ratings[j].movieid==movieid){
            int otherid=Ratings[j].userid;//其他对该电影评分的人 
            int otherscore=Ratings[j].score;//其评分
            float s=x*(float(otherscore)-users[otherid].avgscore);
            if(sims.find(otherid)!=sims.end()) {
                float t=sims[otherid];sims[otherid]=t+s;
            }
            else sims[otherid]=s;
            j++;
        }
    }
}
void inituserSim(){
    for(int i=0;i<USERNUM;i++){
        
}

int main(){
    //    initUserMovieScoreLst(true);
    initdata();
    return 1;
}

