#include <fstream>
#include <algorithm>
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <map>
#include "handle_data.h"
using namespace std;
#define cnt_rating 100480507
map<int ,int> UserIdx;
void readRatingData();
struct Rating {
    int movieid, userid, rating, next;
    void init(int _movieid, int _userid, int _rating) {
        movieid = _movieid, userid = _userid, rating = _rating;
    }
    //rating 按照用户id和评分排序
    bool operator <(const Rating &a) const {
        return userid < a.userid || userid == a.userid && rating > a.rating;
    }
}ratings[cnt_rating];

//创建用户连续编号
void initUserIdx(){
    //读取useridx（useridx连续编号）到userIdx map数组
    ifstream ifile("datas/useridx.data");
    UserIdx.clear();
    if(ifile.good()){
        printf("reading datas/useridx\n");
        int a,b;
        while(ifile>>a>>b){
            UserIdx[a]=b;
        }
        return;
    }
    //创建连续编号  
    readRatingData();
    int curidx=0;
    printf("create continus user idx\n");
    for(int i = cnt_rating - 1; i >= 0; --i) {
        Rating rate=ratings[i];
        if(UserIdx.find(rate.userid)==UserIdx.end()) {
            printf("%d:%d\n",rate.userid,curidx);
            UserIdx[rate.userid]=curidx++;
        }
    }
    ofstream ofile("datas/useridx" );
    for(map<int,int>::iterator it=UserIdx.begin();it!=UserIdx.end();it++)
        ofile<<it->first<<it->second;
    ofile.flush();
}

//private  
////读取rating.data
void readRatingData(){
    FILE *ratingDataFile=fopen("datas/sortedrating.data","r");
    if(ratingDataFile==NULL){
        ratingDataFile=fopen("datas/rating.data","r");
        FILE* sortedRatingDataFile=fopen("datas/rating.data","wb");
        fread(ratings, sizeof(Rating), cnt_rating, ratingDataFile);
        fclose(ratingDataFile);

        //对ratingdata按照（userid，rating）进行排序
        sort(ratings,ratings+cnt_rating);
        fwrite(ratings,sizeof(Rating),cnt_rating,sortedRatingDataFile);
        fclose(sortedRatingDataFile);
    }else {
        fread(ratings,sizeof(Rating),cnt_rating,ratingDataFile);
        fclose(ratingDataFile);
    }
}

float UserAvgScore[USERNUM];
void initUserAvgScore(bool create){
    FILE *userAvgScoreFile=fopen("datas/useravgscore.data","r");
    if(create)  fclose(userAvgScoreFile);
    if(userAvgScoreFile==NULL||create){
        //用户平均分
        for(int i=0;i<USERNUM;i++){
            int score=0;int num=0;
            for(list<movie_score>::iterator it= UserMovieScoreLst[i].begin();
                    it!= UserMovieScoreLst[i].end();it++)
            score+=it->score,num++;
            UserAvgScore[i]=score==0?0:(float)score/num;
        }
        //写回
        FILE *userAvgScoreFile=fopen("datas/useravgscore.data","wb");
        fwrite(UserAvgScore,sizeof(float),USERNUM,userAvgScoreFile);
       // flush(userAvgScoreFile);
        fclose(userAvgScoreFile);
        return;
    }
    fread(UserAvgScore,sizeof(float),USERNUM,userAvgScoreFile);
}

//获取用户评分的电影列表
void initUserMovieScoreLst(bool create){
    FILE *userMovieScoreLstFile=fopen("datas/usermoviescorelst.data","r");
    if(create)  fclose(userMovieScoreLstFile);
    if(userMovieScoreLstFile==NULL||create){
        //创建用户评分电影文件
       
        readRatingData();
        //从rating.data读到MovieuserList
        for(int i = cnt_rating - 1; i >= 0; --i) {
            Rating rate=ratings[i];
            int userid=rate.userid;int movieid=rate.movieid;int rank=rate.rating;
            printf("userid:%d movieid:%d rating:%d\n",userid,movieid,rank);
            UserMovieScoreLst[UserIdx[userid]].push_back(movie_score(movieid,rank));
        }
        int UserMovieScoreLstIdx[USERNUM];
        memset((void*)UserMovieScoreLstIdx,0,USERNUM*sizeof(int));
        ofstream ofile("datas/usermoviescorelst.data",ios::binary|ios::out);
        ofstream _ofile("datas/usermoviescorelstidx.data",ios::binary|ios::out);
        int p=0;
        //以 userid：movieid,score,形式存储   
        for(int i=0;i<USERNUM;i++){
            ofile<<i;p+=4;
            for(list<movie_score>::iterator it= UserMovieScoreLst[i].begin();
                    it!= UserMovieScoreLst[i].end();it++)
                ofile<<it->movieid<<it->score,p+=8;
            UserMovieScoreLstIdx[i]=p;
        }
        ofile.flush();
        //上述文件的索引
        for(int i=0;i<USERNUM;i++)
            _ofile<<UserMovieScoreLstIdx[i];
        _ofile.flush();
    }
}

void inituserSim(int i,int j){

}

int main(){
//  initUserIdx();
//    initUserMovieScoreLst(true);
    readRatingData();
    return 1;
}

