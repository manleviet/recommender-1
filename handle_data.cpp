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


////读取rating.data，并按照用户id和评分排序
//排序结果保存到sortedrating.data
void readRatingData(){
    FILE *sortedRatingDataFile=fopen("datas/sortedrating.data","r");

    if(sortedRatingDataFile==NULL){
        //创建sortedrating.data
        FILE *ratingDataFile=fopen("datas/rating.data","r");
        sortedRatingDataFile=fopen("datas/sortedrating.data","wb");
        fread(Ratings, sizeof(Rating), cnt_rating, ratingDataFile);
        fclose(ratingDataFile);

        //对ratingdata按照（userid，rating）进行排序
        sort(Ratings,Ratings+cnt_rating);
        for(int i=0;i<1000;i++)
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


int UserIdx[USERNUM];//用户旧编号数组（从小到大）
map<int ,int> UserRevertedIdx;//反向索引：用户旧的编号-》新的编号
//创建用户连续编号
//记录用户旧的编号数组 到datas/useridx.data
void initUserIdx(){
    FILE *ifile=fopen("datas/useridx.data","r");

    if(ifile!=NULL){
        //从文件读取编号到userIdx数组
        printf("reading datas/useridx.data\n");
        fread(UserIdx,sizeof(int),USERNUM,ifile);
        fclose(ifile);

        //创建反向索引（旧编号-》新编号）
        for(int i=0;i<USERNUM;i++)
            UserRevertedIdx[UserIdx[i]]=i;
        return;
    }

    else{
        //创建连续编号  
        printf("create continus user idx\n");
        int pre_userid=-1;
        int curidx=0;
        for(int i = 0 ; i<cnt_rating; i++) {
            Rating rate=Ratings[i];
            if(pre_userid!=rate.userid) {
                printf("curidx:%d  userid:%d\n",curidx,rate.userid);
                pre_userid=rate.userid;
                UserIdx[curidx]=rate.userid;
                UserRevertedIdx[rate.userid]=curidx++;
            }
        }
        //写到文件
        ifile=fopen("datas/useridx.data","wb");
        fwrite(UserIdx,sizeof(int),USERNUM,ifile);
        fclose(ifile);
    }
}


float UserSigma[USERNUM];//记录用户的斯格玛
float UserAvgScore[USERNUM];//记录用户的平均评分
//获取计算用户的平均分和sigma
void initUserAvgScore(bool create){
    FILE *userAvgScoreFile=fopen("datas/useravgscore.data","r");
    FILE *userSigmaFile=fopen("datas/usersigma.data","r");

    if(userAvgScoreFile==NULL||create){
        //创建用户平均分和斯格玛文件
        for(int i=0;i<USERNUM;i++){
            float score=0;int num=0;float sigma=0;
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
        fclose(userAvgScoreFile);
        return;
    }

    else{
        //从文件读取平均分和斯格玛
        fread(UserAvgScore,sizeof(float),USERNUM,userAvgScoreFile);
        fread(UserSigma,sizeof(float),USERNUM,userSigmaFile);
        fclose(userAvgScoreFile);
        fclose(userSigmaFile);
    }
}

//获取用户的评分列表(ratinglist)
void initUserRatingLst(bool create){
    FILE *userMovieScoreLstFile=fopen("datas/usermoviescorelst.data","r");
    if(create)  fclose(userMovieScoreLstFile);
    if(userMovieScoreLstFile==NULL||create){
        //创建用户评分电影文件

        //从rating.data读到MovieuserList
        for(int i = cnt_rating - 1; i >= 0; --i) {
            Rating rate=Ratings[i];
            int userid=rate.userid;int movieid=rate.movieid;int score=rate.score;
            printf("userid:%d movieid:%d rating:%d\n",userid,movieid,score);
            UserMovieScoreLst[UserRevertedIdx[userid]].push_back(movie_score(movieid,score));
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
    //    initUserMovieScoreLst(true);
    readRatingData();
    initUserIdx();
    return 1;
}

