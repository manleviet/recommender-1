#include <iostream>
#include <list>
using namespace std;
struct user_sim{
    int i;
    int j;
    float sim;
};
//电影评分
struct movie_score{
    int movieid;
    float score;
    movie_score(int a,float b):movieid(a),score(b){}
};
//
struct user_score{
    int userid;
    float score;
    user_score(int a,float b):userid(a),score(b){}
};

//相似度
struct similarity{
    int i;
    int j;
    float sim;
};
#define USERNUM 480190
#define MOVIENUM 17780
//#define RATINGNUM
//rating.data -> MovieUserLst
list<struct user_score> MovieUserLst[MOVIENUM];//看过电影i的用户id list

//item based
#ifdef MOVIE_BASED
//电影斯格玛
float movie_sigma[MOVIENUM];//看过这个电影的所有人对该电影评分减去他给出的平均分的平方和
//电影之间的相似度
list<struct similarity> MovieSimLst[MOVIENUM];//记录电影和其他电影的相似度
        //movie i,j之间相似度=movie i被user[u]评分*movie j被user
//电影平均分
float movie_avg_score[MOVIENUM];
//predict
#endif


#define USER_BASED

//user based
#ifdef USER_BASED
//用户斯格玛
float user_sigma[USERNUM];//看过这个电影的所有人对该电影评分减去他给出的平均分的平方和
list<struct movie_score> UserMovieScoreLst[USERNUM];//用户看过的电影及评分列表
//电影之间的相似度
list<struct similarity> UserSimLst[USERNUM];//记录电影和其他电影的相似度
        //movie i,j之间相似度=movie i被user[u]评分*movie j被user
//用户给出的平均分
float user_avg_score[USERNUM];
#endif
