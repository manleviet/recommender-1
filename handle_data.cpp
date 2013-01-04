#include <fstream>
#include <algorithm>
#include <math.h>
#include <iostream>
#include <string.h>
#include <stdio.h>
#include <map>
#include "handle_data.h"
using namespace std;
#define cnt_rating 100480507
void readRatingData();
struct Rating {
    int movieid, userid, score, next;
    void init(int _movieid, int _userid, int _score) {
        movieid = _movieid, userid = _userid, score = _score;
    }
    //score 按照用户id和评分排序
    bool operator <(const Rating &a) const {
        return userid < a.userid || userid == a.userid && score > a.score;
    }
}Ratings[cnt_rating];
int UserRatingsHead[USERNUM];//记录user在Ratings数组的开始下标
int MovieRatingsHead[MOVIENUM];//记录user在Ratings数组的开始下标

map<int ,int> UserIdx;
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
        Rating rate=Ratings[i];
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

////读取rating.data，并按照用户id和评分排序
//排序结果保存到sortedrating.data
void readRatingData(){
    FILE *sortedRatingDataFile=fopen("datas/sortedrating.data","r");

    if(sortedRatingDataFile==NULL){
        FILE *ratingDataFile=fopen("datas/rating.data","r");
        sortedRatingDataFile=fopen("datas/sortedrating.data","wb");
        fread(Ratings, sizeof(Rating), cnt_rating, ratingDataFile);
        fclose(ratingDataFile);

        //对ratingdata按照（userid，rating）进行排序
        sort(Ratings,Ratings+cnt_rating);
        for(int i=0;i<100000;i++)
            printf("%d:%d:%d\n",Ratings[i].userid,Ratings[i].score,Ratings[i].movieid);

        //将排序结果输出到文件
        fwrite(Ratings,sizeof(Rating),cnt_rating,sortedRatingDataFile);
        fclose(sortedRatingDataFile);
        printf("sorted rating.data \n");
    }else {

        //读取排序结果
        fread(Ratings,sizeof(Rating),cnt_rating,sortedRatingDataFile);
        fclose(sortedRatingDataFile);
    }
}

float UserSigma[USERNUM];
float UserAvgScore[USERNUM];
//获取用户的平均分和sigma
void initUserAvgScore(bool create){
    FILE *userAvgScoreFile=fopen("datas/useravgscore.data","r");
    FILE *userSigmaFile=fopen("datas/usersigma.data","r");
    if(create)  fclose(userAvgScoreFile);
    if(userAvgScoreFile==NULL||create){
        //用户平均分
        for(int i=0;i<USERNUM;i++){
            float score=0;int num=0;float sigma=0;
            //            for(list<movie_score>::iterator it= UserMovieScoreLst[i].begin();
            //                    it!= UserMovieScoreLst[i].end();it++)
            int head=UserRatingsHead[i];
            //用户评分列表非空
            if(head>=0){
                int userid=Ratings[head].userid;
                for(int j=head;;j++){
                    if(Ratings[j].userid!=userid) break;
                    score+=Ratings[j].score,num++;
                }

                //计算平均分
                UserAvgScore[i]=score==0?0:(float)score/num;
                for(int j=head;;j++){
                    //计算斯格玛：（用户的每个电影评分-平均评分）的平方和，再开根号。
                    //斯格玛为用户的电影评分向量的模
                    if(Ratings[j].userid!=userid) break;
                    sigma+=pow(Ratings[j].score-UserAvgScore[i],2);
                }
                UserSigma[i]=sqrt(sigma);//开根号       
            }

            else {
                printf("useridx:%d  no movie rated\n",i);
                UserAvgScore[i]=0,UserSigma[i]=0;
            }

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

//获取用户的评分列表(ratinglist)
void initUserRatingLst(bool create){
    FILE *userMovieScoreLstFile=fopen("datas/usermoviescorelst.data","r");
    if(create)  fclose(userMovieScoreLstFile);
    if(userMovieScoreLstFile==NULL||create){
        //创建用户评分电影文件

        readRatingData();
        //从rating.data读到MovieuserList
        for(int i = cnt_rating - 1; i >= 0; --i) {
            Rating rate=Ratings[i];
            int userid=rate.userid;int movieid=rate.movieid;int score=rate.score;
            printf("userid:%d movieid:%d rating:%d\n",userid,movieid,score);
            UserMovieScoreLst[UserIdx[userid]].push_back(movie_score(movieid,score));
        }
        int UserMovieScoreLstIdx[USERNUM];
        memset((void*)UserMovieScoreLstIdx,0,USERNUM*sizeof(int));
        ofstream ofile("datas/usermoviescorelst.data",ios::binary|ios::out);
        ofstream _ofile("datas/usermoviescorelstidx.data",ios::binary|ios::out);
        int p=0;
        //以 userid：movieid,score,形式存储   
        for(int i=0;i<USERNUM;i++){
            ofile<<i<<0;p+=2;
            for(list<movie_score>::iterator it= UserMovieScoreLst[i].begin();
                    it!= UserMovieScoreLst[i].end();it++)
                ofile<<it->movieid<<it->score,p+=2;
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

