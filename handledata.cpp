#include <stdio.h>
#include <vector>
#include <algorithm>
#include <unistd.h>
#include <math.h>
#include <string.h>
#include <map>
using namespace std;
#define RATINGNUM 100480507
#define MOVIENUM 17770 
#define USERNUM 480189
#define RATIO  1.1//计算相似度，系数ratio
#define HOODSIZE 30//最多保存30个邻居
#define THRESHOLD 0.25//相似度阈值，不能低于0.25
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
}Ratings[RATINGNUM];
//注意：movie和user从1开始编号
struct Movie {
    int head, end, cnt;//Ratings起始下标head和电影评分个数cnt
    double dat_score;
    double avgscore;//平均评分
    double sigma;
    Movie () {
        //初始化
        head = -1, cnt = 0, avgscore = 0, dat_score = 0; sigma = 0 ;
    }
}movies[MOVIENUM + 1];
struct User {
    int head, cnt;//Ratings下标和电影评分个数
    double dat_score;
    double avgscore;
    double sigma;
    User () {
        //初始化
        head = -1, cnt = 0, avgscore = 0, dat_score = 0;sigma = 0 ;
    }
}users[USERNUM + 1];
struct SimIdx{//用户相似度记录的索引
    int userid;//用户id
    int begin;//用户i的相似度记录UserSim[]的开始下标
    int cnt;//记录个数
    SimIdx(){userid=-1,begin=-1,cnt=-1;}
}simidxs[USERNUM+1];

struct UserSim{
    int userid;//用户id
    double sim;//相似度
}sims[310000];//缓存用户相似度记录
void getUserSim(int l,map<int,double> &sims);
void testUserMovieLst(int l){   
    printf("\nuser:%d movies:\n",l);
    int head=users[l].head;
    while(head>=0) printf("%d  ",Ratings[head].movieid),head=Ratings[head].next;
}
void testMovieUserLst(int l){
    printf("\nmovie:%d users:\n",l);
    int head=movies[l].head;
    while(Ratings[head].movieid==l) printf("%d  ",Ratings[head].userid),head++;
}

void initdata(){
    FILE *ratingDataFile=fopen("datas/rating.data","r");
    if(ratingDataFile==NULL) {printf("open rating.data error\n");return;}
    fread(Ratings, sizeof(Rating), RATINGNUM, ratingDataFile);
    fclose(ratingDataFile);

    printf("check if users.data and movies.data exists\n");
    if(access("datas/users.data",0)!=-1 && access("datas/movies.data",0)!=-1){
        printf("exists\n");
        FILE* usersDataFile=fopen("datas/users.data","r");
        fread(users,sizeof(User),USERNUM+1,usersDataFile);
        fclose(usersDataFile);
        FILE* moviesDataFile=fopen("datas/movies.data","r");
        fread(movies,sizeof(Movie),MOVIENUM+1,moviesDataFile);
        fclose(moviesDataFile);
    }else{
        //计算users movies

        //预处理过后的数据
        for(int i =RATINGNUM-1; i>=0; --i) {
            int movieid = Ratings[i].movieid;
            int userid = Ratings[i].userid;
            int score = Ratings[i].score;
            movies[movieid].head = i;
            movies[movieid].avgscore+=score;
            ++ movies[movieid].cnt;

            users[userid].head = i;
            users[userid].avgscore += score;
            ++ users[userid].cnt;//用户评分的电影个数
        }

        //计算用户给出平均分、斯格玛
        printf("user avg score and sigma:\n");
        for(int i=1;i<=USERNUM;i++){
            if(i<10) printf("\nuser:%d\n",i);
            users[i].avgscore=users[i].cnt==0?0:(double)users[i].avgscore/users[i].cnt;
            int head=users[i].head;
            if(head<0) printf("userid:%d head error:%d\n",i, head);
            while(head>=0){
                if(i<10)printf("%d ",head);
                // if(i<10) printf("user id:%d score:%d avg:%f\n",i,Ratings[head].score,users[i].avgscore);
                // 为了避免sigma计算得到0，所以乘以RATIO 
                users[i].sigma+=pow(RATIO*Ratings[head].score-users[i].avgscore,2);
                head=Ratings[head].next;
            }
            users[i].sigma=sqrt(users[i].sigma);
            if(i<10||i>USERNUM-10) printf("userid:%d avg:%f sigma:%f\n",i,users[i].avgscore,users[i].sigma);
        }

        //计算电影获得平均分、斯格玛
        printf("movie avg score and sigma:\n");
        for(int i=1;i<=MOVIENUM;i++){
            if(i<10)printf("\nmovie:%d\n",i);
            movies[i].avgscore=movies[i].cnt==0?0:(double)movies[i].avgscore/movies[i].cnt;
            int head=movies[i].head;
            if(i<10)printf("%d ",head);
            while(Ratings[head].movieid==i){
                //   if(i<10) printf("movie id:%d score:%d avg:%f\n",i,Ratings[head].score,movies[i].avgscore);
                // 为了避免sigma计算得到0，所以乘以RATIO 
                movies[i].sigma+=pow(RATIO*Ratings[head].score-movies[i].avgscore,2);
                head++;
            }
            movies[i].sigma=sqrt(movies[i].sigma);
            if(i<10||i>MOVIENUM-10) printf("avg:%f sigma:%f\n",movies[i].avgscore,movies[i].sigma);
        }

        //输出users、movies
        printf("output users to users.data\n");
        FILE *fout=fopen("datas/users.data","wb");
        fwrite(users,sizeof(User),USERNUM+1,fout);
        fclose(fout);
        printf("output movies to movies.data\n");
        fout=fopen("datas/movies.data","wb");
        fwrite(movies,sizeof(Movie),MOVIENUM+1,fout);
        fclose(fout);
    }  
}

//计算用户之间的相似度
//并保存到sim.data和simidx.data
typedef pair<int,double> PAIR;  
int cmp(const PAIR& x, const PAIR& y)  {  return x.second > y.second;  }  
void initSim(){
    printf("computing the similarity of users\n");
    //计算用户相似度
    //取得相似度最高的30个以内的邻居，保存他们的相似度。
    //数据规模为48w*0.003w*12=1.2亿字节，大约需要180MB的存储
    //对于任意一部电影，大约会被用户的30个邻居中的5个看过
    FILE *simFile=fopen("datas/sim.data","wba");//记录用户之间相似度的文件
    FILE *simIdxFile=fopen("datas/simidx.data","wba");//记录用户相似度的文件索引
    int curidx=0,baseidx=0;
    for(int i=1;i<=1;i++){
        //当存储超过300000个相似度纪录时 flush memory
        if(curidx>300000) {
            fwrite(sims,sizeof(UserSim),curidx,simFile);
            baseidx+=curidx;
            curidx=0;
        }

        printf("user %d:\n",i);
        map<int,double> res; vector<PAIR> vec;  
        getUserSim(i,res);
        for (map<int,double>::iterator curr = res.begin(); curr != res.end(); ++curr)  
            vec.push_back(make_pair(curr->first, curr->second));  
        sort(vec.begin(),vec.end(),cmp);

        int num=0;
        simidxs[i].begin=curidx+baseidx;simidxs[i].userid=i;
        for(vector<PAIR>::iterator it=vec.begin();it!=vec.end();it++){
            if(it->second>THRESHOLD){//如果相似度大于阈值
                sims[curidx].userid=it->first;
                sims[curidx].sim=it->second;
                num++;curidx++;
                if(num>=HOODSIZE) break;//邻居记录个数不能超过限额（30）
            }
            else break;
        }
        simidxs[i].cnt=num;
    }
    fwrite(sims,sizeof(UserSim),curidx,simFile);
    fwrite(simidxs,sizeof(SimIdx),USERNUM+1,simIdxFile);
    fclose(simFile);fclose(simIdxFile);
    //    testUserMovieLst(1);
    //    testMovieUserLst(1);
}   

//获得计算两个人的相似度的
//皮尔森系数公式的分母（sigma i * sigma j)
double getSimBase(int i,int j){
    //如果sigma i或j=0，则返回0
    return users[i].sigma*users[j].sigma;
}

//协同过滤
//计算用户l对movie的预测评分
double predict(int l,int m,FILE *fin){
    int idx=simidxs[l].begin;
    int cnt=simidxs[l].cnt;
    return 0;
}
//计算用户i和其他人的相似度(皮尔森相关系数)
void getUserSim(int l,map<int,double>& sims){

    int i=users[l].head;
    while(i>0)
    {
        Rating rate=Ratings[i];int score=rate.score;int movieid=rate.movieid;
        double x=(double)score*RATIO-users[l].avgscore;
        int j=movies[movieid].head;
        //        printf("\nmovie:%d\n",movieid);
        //        printf(" neighbour:%d ",Ratings[j].userid);
        while(Ratings[j].movieid==movieid){//对该电影，找到所有评论过的人，获得其分子
            int otherid=Ratings[j].userid;//其他对该电影评分的人 
            int otherscore=Ratings[j].score;//其评分
            double s=x*(double(otherscore)*RATIO-users[otherid].avgscore)/getSimBase(otherid,l);
            if(sims.find(otherid)!=sims.end()) {
                double t=sims[otherid];sims[otherid]=t+s;
            }
            else sims[otherid]=s;
            j++;
        }
        i=Ratings[i].next;//用户评论过的下一个电影
    }
}
void inituserSim(){
}

int main(){
    //    initUserMovieScoreLst(true);
    initdata();
    initSim();  
    return 1;
}

