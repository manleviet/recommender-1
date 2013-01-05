#include <stdio.h>
#include <queue>
#include <map>
#include <algorithm>
#include <unistd.h>
#include <math.h>
#include <string.h>
using namespace std;
#define RATINGNUM 100480507
#define MOVIENUM 17770 
#define USERNUM 480189
#define RATIO  1.1//计算相似度，系数ratio
#define HOODSIZE 10//最多保存30个邻居
#define THRESHOLD 0.3//相似度阈值，不能低于0.3
#define SIMNUM 4802000
//从预处理后的文件rating.data读入的数据：


//注意：movie和user从1开始编号
struct Movie {
    int head, end, cnt;//Ratings起始下标head和电影评分个数cnt
    float dat_score;
    float avgscore;//平均评分
    float sigma;
    //    Movie () {
    //        //初始化
    //        head = -1, cnt = 0, avgscore = 0, dat_score = 0; sigma = 0 ;
    //    }
}movies[MOVIENUM + 2];
struct User {
    int head, cnt;//Ratings下标和电影评分个数
    float dat_score;
    float avgscore;
    float sigma;
    //    User () {
    //        //初始化
    //        head = -1, cnt = 0, avgscore = 0, dat_score = 0;sigma = 0 ;
    //    }
}users[USERNUM + 1];
struct Rating {
    int movieid, userid, score, next;
    void init(int _movieid, int _userid, int _score) {
        movieid = _movieid, userid = _userid, score = _score;
    }
    //同一个movie的评分 按照(用户score-用户平均score)/用户斯格玛的绝对值从大到小排序
    bool operator <(const Rating &a) const {
        float s=(score-users[userid].avgscore)/users[userid].sigma;
        float t=(a.score-users[a.userid].avgscore)/users[a.userid].sigma;
        return fabs(s)>fabs(t);
    }
}Ratings[RATINGNUM];
//struct SimIdx{//用户相似度记录的索引
//    int userid;//用户id
//    int cnt;//记录个数
//    //    SimIdx(){userid=-1,begin=-1,cnt=-1;}
//}simidxs[USERNUM+1];
struct UserSim{
    int userid;//用户id
    float sim;//相似度
    void init(){userid=0;sim=0;}
}sims[SIMNUM];//缓存用户相似度记录
void getUserSim(int l,map<int,float> &sims);
void testUserMovieLst(int l){   
    printf("\nuser:%d\n",l);
    int head=users[l].head;
    while(head>=0) printf("movie:%d  score:%d\n",Ratings[head].movieid,Ratings[head].score),head=Ratings[head].next;
}
void testMovieUserLst(int l){
    printf("\nmovie:%d\n",l);
    int head=movies[l].head;
    while(Ratings[head].movieid==l) {
        int userid=Ratings[head].userid;
        printf("user:%d  simratio:%f\n",Ratings[head].userid,
                (Ratings[head].score-users[userid].avgscore)/users[userid].sigma),head++;
    }
}

void initdata(){

    printf("check if sortedrating.data exists\n");
    bool sorted=(access("datas/sortedrating.data",0)!=-1);//不存在重新排序的rating.data，则准备重新创建

    FILE *ratingDataFile;
    if(!sorted)  ratingDataFile=fopen("datas/rating.data","r");
    else    ratingDataFile=fopen("datas/sortedrating.data","r");
    if(ratingDataFile==NULL) {printf("open [sorted]rating.data error,\
            please run data2bin input/training_set/ input/qualifying.txt to generate it\n");
            return;
    }
    printf("reading rating.data,this will take some time\n");
    fread(Ratings, sizeof(Rating), RATINGNUM, ratingDataFile);
    fclose(ratingDataFile);
    printf("read rating.data successfully\n");

    printf("check if users.data and movies.data exists\n");
    if(access("datas/users.data",0)!=-1 && access("datas/movies.data",0)!=-1){
        printf("exists\n");
        FILE* usersDataFile=fopen("datas/users.data","r");
        fread(users,sizeof(User),USERNUM+1,usersDataFile);
        fclose(usersDataFile);
        FILE* moviesDataFile=fopen("datas/movies.data","r");
        fread(movies,sizeof(Movie),MOVIENUM+1,moviesDataFile);
        fclose(moviesDataFile);
        movies[MOVIENUM+1].head=RATINGNUM;
    }else{
        //计算users movies

        printf("will generate users.data and movies.data\n");
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
        for(int i=1;i<=USERNUM;i++){
            if(i%10000==0) printf("\nuser:%d\n",i);
            users[i].avgscore=users[i].cnt==0?0:(float)users[i].avgscore/users[i].cnt;
            int head=users[i].head;
            if(head<0) printf("userid:%d head error:%d\n",i, head);
            while(head>=0){
                //if(i<10)printf("%d ",head);
                // if(i<10) printf("user id:%d score:%d avg:%f\n",i,Ratings[head].score,users[i].avgscore);
                // 为了避免sigma计算得到0，所以乘以RATIO 
                users[i].sigma+=pow(RATIO*Ratings[head].score-users[i].avgscore,2);
                head=Ratings[head].next;
            }
            users[i].sigma=sqrt(users[i].sigma);
            if(i%10000==0) printf("userid:%d avg:%f sigma:%f\n",i,users[i].avgscore,users[i].sigma);
        }

        //计算电影获得平均分、斯格玛
        printf("movie avg score and sigma:\n");
        for(int i=1;i<=MOVIENUM;i++){
            if(i%1000==0)printf("\nmovie:%d\n",i);
            movies[i].avgscore=movies[i].cnt==0?0:(float)movies[i].avgscore/movies[i].cnt;
            int head=movies[i].head;
            //if(i<10)printf("%d ",head);
            while(Ratings[head].movieid==i){
                //   if(i<10) printf("movie id:%d score:%d avg:%f\n",i,Ratings[head].score,movies[i].avgscore);
                // 为了避免sigma计算得到0，所以乘以RATIO 
                movies[i].sigma+=pow(RATIO*Ratings[head].score-movies[i].avgscore,2);
                head++;
            }
            movies[i].sigma=sqrt(movies[i].sigma);
            if(i%1000==0) printf("avg:%f sigma:%f\n",movies[i].avgscore,movies[i].sigma);
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

    //重新计算排序后的ratings并输出到sortedrating.data\users.data
    if(!sorted)
    {
        printf("begin sorting ratings.data,this will take some time\n");
        for(int i=1;i<=MOVIENUM;i++){
            int head=movies[i].head;
            int tail=movies[i+1].head;
            if(Ratings[head].movieid!=i||Ratings[tail-1].movieid!=i)
                printf("movie:%d head:%d tail:%d error\n",i,head,tail);
            sort(Ratings+head,Ratings+tail);
        }
        for(int i=1;i<=USERNUM;i++)
            users[i].head=-1;
        for(int i = RATINGNUM - 1; i >= 0; --i) {
            Ratings[i].next = users[Ratings[i].userid].head;
            users[Ratings[i].userid].head = i;
        }
        printf("output sortedratings to sortedrating.data\n");
        FILE* usersDataFile=fopen("datas/users.data","wb");
        fwrite(users,sizeof(User),USERNUM+1,usersDataFile);
        fclose(usersDataFile);

        FILE *sortedRatingFile = fopen("datas/sortedrating.data", "wb");
        fwrite(Ratings,sizeof(Rating),RATINGNUM,sortedRatingFile);
        fclose(sortedRatingFile);
    }
    printf("all data has been initialized!\n");
}

//计算用户之间的相似度
//并保存到sim.data和simidx.data
typedef pair<int,float> PAIR;  
//struct cmp{
//    int cmp(const PAIR& x, const PAIR& y)  {  return x.second > y.second;  }  
//};
class cmp{
    public :
        int operator()(const PAIR& x, const PAIR& y)  {  return x.second < y.second;  }  
};
void initSim(){
    printf("computing the similarity of users\n");
    //计算用户相似度
    //取得相似度最高的30个以内的邻居，保存他们的相似度。
    //数据规模为48w*0.003w*12=1.2亿字节，大约需要180MB的存储
    //对于任意一部电影，大约会被用户的30个邻居中的5个看过
    FILE *simFile=fopen("datas/sim.data","wba");//记录用户之间相似度的文件
    //    FILE *simIdxFile=fopen("datas/simidx.data","wba");//记录用户相似度的文件索引
    int curidx=0;//,baseidx=0;
    map<int,float> res; 
    //        vector<PAIR> vec;  
    for(int i=1;i<=USERNUM;i++){
        //当存储超过300000个相似度纪录时  溢出！
        //        if(curidx>=SIMNUM) {
        //            printf("curidx:%d is out of buffer\n",curidx);
        //            return;
        //        }
        // printf("user %d:\n",i);
        res.clear(); 
        getUserSim(i,res);
        priority_queue<PAIR,vector<PAIR>,cmp > vec;
        for (map<int,float>::iterator curr = res.begin(); curr != res.end(); ++curr)  
            //            vec.push_back(make_pair(curr->first, curr->second));  
            vec.push(make_pair(curr->first, curr->second));  
        //        sort(vec.begin(),vec.end(),cmp);
        int num=0;
        curidx=i*10;//+baseidx;
        //        for(vector<PAIR>::iterator it=vec.begin();it!=vec.end();it++){
        while(!vec.empty()){
            //            if(it->second>THRESHOLD){//如果相似度大于阈值
            PAIR it=vec.top();vec.pop();
            sims[curidx].userid=it.first;
            sims[curidx++].sim=it.second;
            num++;
            if(num>=HOODSIZE) break;//邻居记录个数不能超过限额（10）
            //            }
            //            else break;
        }
        if(num<HOODSIZE)  sims[curidx++].init();
        if(i%1000==0){ //48w/0.1w=480
            printf("user:%dK num:%d\n",i/1000,num);
            for(int j=0;j<num;j++) 
                printf("neighborid:%d sim:%f\n",sims[i*10+j].userid,sims[i*10+j].sim);
        }
    }
    printf("begin write usersim\n");
    fwrite(sims,sizeof(UserSim),curidx,simFile);
    fclose(simFile);
    }   

    //获得计算两个人的相似度的
    //皮尔森系数公式的分母（sigma i * sigma j)
    float getSimBase(int i,int j){
        //如果sigma i或j=0，则返回0
        return users[i].sigma*users[j].sigma;
    }

    //协同过滤
    //计算用户l对movie的预测评分
    float predict(int l,int m,FILE *fin){
        //        int cnt=simidxs[l].cnt;
        //    fseek(idx);
        //    fread(usersims,sizeof(UserSim),cnt,fin);//从文件里读出相似度
        //    fseek(idx);
        return 0;
    }
    //计算用户i和其他人的相似度,获得邻居(皮尔森相关系数)
    //注意对于每个用户运算复杂度不能超过1000
    void getUserSim(int l,map<int,float>& sims){

        int i=users[l].head;
        while(i>0)
        {
            Rating rate=Ratings[i];int score=rate.score;int movieid=rate.movieid;
            float x=(float)score*RATIO-users[l].avgscore;
            int j=movies[movieid].head;
            //        printf("\nmovie:%d\n",movieid);
            //        printf(" neighbour:%d ",Ratings[j].userid);
            //        while(Ratings[j].movieid==movieid){//对该电影，找到评论过的人，找出最高分和最低分，获得其分子
            //            int otherid=Ratings[j].userid;//其他对该电影评分的人 
            //            int otherscore=Ratings[j].score;//其评分
            //            float s=x*(float(otherscore)*RATIO-users[otherid].avgscore)/getSimBase(otherid,l);
            //            if(sims.find(otherid)!=sims.end()) {
            //                float t=sims[otherid];sims[otherid]=t+s;
            //            }
            //            else sims[otherid]=s;
            //            j++;
            //        }
            int usernum=movies[l].cnt;
            int num=(int)(sqrt(usernum)*20);//该公式用来限制从电影中添加邻居的个数
            while(j<movies[movieid+1].head) {
                int otherid=Ratings[j].userid;//其他对该电影评分的人 
                int otherscore=Ratings[j].score;//其评分
                float s=(x/users[l].sigma)*
                    ((otherscore-users[otherid].avgscore)/users[otherid].sigma);
                if(sims.find(otherid)!=sims.end()) {
                    float t=sims[otherid];sims[otherid]=t+s;
                } else sims[otherid]=s;
                j+=num;
            }
            i=Ratings[i].next;//用户评论过的下一个电影
        }
    }
    void inituserSim(){
    }

    int main(){
        //    initUserMovieScoreLst(true);
        initdata();
        //        initSim();  
        //        testUserMovieLst(1);
        testMovieUserLst(1);
        return 1;
    }

