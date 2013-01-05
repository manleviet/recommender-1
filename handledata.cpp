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
#define RATIO  1//计算相似度，系数ratio
#define HOODSIZE 30//最多保存30个邻居
#define THRESHOLD 0.3//相似度阈值，不能低于0.3
#define SIMNUM HOODSIZE*(USERNUM+1)
//从预处理后的文件rating.data读入的数据：

//注意：movie和user从1开始编号
struct Movie {//某个电影的属性
    int head, end, cnt;//Ratings起始下标head、电影评分个数cnt
    float dat_score;
    float avgscore;//平均评分
    float sigma;//皮尔森系数的分母
}movies[MOVIENUM + 2];

struct User {//某个用户的属性
    int oldid;//保存旧的编号
    int head, cnt;
    float dat_score;
    float avgscore;
    float sigma;
}users[USERNUM + 1];

struct Rating {//一项评分
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
//
//struct UserSim{
//    int userid;//用户id
//    float sim;//相似度
//    void init(){userid=0;sim=0;}
//}sims[SIMNUM];//缓存用户相似度记录

typedef pair<int,float> PAIR;  
class cmp{
    public : int operator()(const PAIR& x, const PAIR& y)  {  return x.second < y.second;  }  
};

void getUserSim(int l,map<int,float> &sims);
//
////测试数据结构是否完好
//void testUserMovieLst(int l){   
//    printf("\nuser:%d\n",l);
//    int head=users[l].head;
//    while(head>=0) printf("movie:%d  score:%d\n",Ratings[head].movieid,Ratings[head].score),head=Ratings[head].next;
//}
//void testMovieUserLst(int l){
//    printf("\nmovie:%d\n",l);
//    int head=movies[l].head;
//    while(Ratings[head].movieid==l) {
//        int userid=Ratings[head].userid;
//        printf("user:%d  simratio:%f\n",Ratings[head].userid,
//                (Ratings[head].score-users[userid].avgscore)/users[userid].sigma),head++;
//    }
//}

//初始化各项数据
void initData(){

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
        memset(users,0,sizeof(users));
        memset(movies,0,sizeof(movies));

        if(access("datas/user_mapback.data",0)!=-1){
            printf("reading user_mapback.data\n");
            int user_mapback[USERNUM+1];
            FILE *fin=fopen("datas/user_mapback.data","r");
            fread(user_mapback,sizeof(int),USERNUM+1,fin);
            fclose(fin);
            printf("read user_mapback.data successfully\n");
            for(int i=1;i<=USERNUM;i++)
                users[i].oldid=user_mapback[i];
        }else {
            printf("ERROR:cannot open user_mapback.data for read,\
                    please run data2bin to generate it!\n");
            return ;
        }

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
            users[i].avgscore=users[i].cnt==0?0:users[i].avgscore/users[i].cnt;
            int head=users[i].head;
            if(head<0) printf("userid:%d head error:%d\n",i, head);
            while(head>=0){
                // 为了避免sigma计算得到0，所以乘以RATIO 
                users[i].sigma+=pow(Ratings[head].score-users[i].avgscore,2);
                head=Ratings[head].next;
            }
            users[i].sigma=sqrt(users[i].sigma);
            if(users[i].sigma==0) {
                printf("user %d sigma=0,will be set to 1\n",i);
                users[i].sigma=1;
            }
            if(i%10000==0) printf("userid:%d avg:%f sigma:%f\n",i,users[i].avgscore,users[i].sigma);
        }

        //计算电影获得平均分、斯格玛
        printf("movie avg score and sigma:\n");
        for(int i=1;i<=MOVIENUM;i++){
            if(i%1000==0)printf("\nmovie:%d\n",i);
            movies[i].avgscore=movies[i].cnt==0?0:movies[i].avgscore/movies[i].cnt;
            int head=movies[i].head;
            while(Ratings[head].movieid==i){
                // 为了避免sigma计算得到0，所以乘以RATIO 
                movies[i].sigma+=pow(Ratings[head].score-movies[i].avgscore,2);
                head++;
            }
            movies[i].sigma=sqrt(movies[i].sigma);
            if(movies[i].sigma==0) {
                printf("movie %d sigma=0,will be set to 1\n",i);
                movies[i].sigma=1;
            }
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

//计算用户之间的相似度,获得邻居
//并保存到sim.data
void initSim(){
    //    //读取邻居文件
    //    if(access("datas/sim.data",0)!=-1){
    //        printf("reading sim.data,this will take some time\n");
    //        FILE *simFile=fopen("datas/sim.data","r");
    //        fread(sims,sizeof(UserSim),SIMNUM,simFile);
    //        fclose(simFile);
    //        printf("read sim.data successfully\n");
    //        return;
    //    }
    //    //初始化相似度为0
    //    memset(sims,0,sizeof(sims));
    //    //创建邻居文件
    //    printf("computing the similarity of users\n");
    //    //计算用户相似度
    //    //取得相似度最高的30个以内的邻居，保存他们的相似度。
    //    //数据规模为48w*0.003w*12=1.2亿字节，大约需要180MB的存储
    //    //对于任意一部电影，大约会被用户的30个邻居中的5个看过
    //    FILE *simFile=fopen("datas/sim.data","wb");//记录用户之间相似度的文件
    //    int curidx=0;
    //    map<int,float> res; 
    //    for(int i=1;i<=USERNUM;i++){
    //        res.clear(); 
    //        getUserSim(i,res);
    //        priority_queue<PAIR,vector<PAIR>,cmp > vec;
    //        for (map<int,float>::iterator curr = res.begin(); curr != res.end(); ++curr)  
    //            vec.push(make_pair(curr->first, curr->second));  
    //        int num=0;
    //        curidx=i*HOODSIZE;
    //        while(!vec.empty()){
    //            //if(it->second>THRESHOLD){//如果相似度大于阈值
    //            PAIR it=vec.top();vec.pop();
    //            sims[curidx].userid=it.first;
    //            sims[curidx++].sim=it.second;
    //            num++;
    //            if(num>=HOODSIZE) break;//邻居记录个数不能超过限额度
    //        }
    //        if(num<HOODSIZE)  sims[curidx++].init();//尾端标记
    //
    //        //输出计算的进度信息...
    //        if(i%1000==0){ //48w/0.1w=480
    //            printf("user:%dK num:%d\n",i/1000,num);
    //            for(int j=0;j<num;j++) 
    //                printf("neighborid:%d sim:%f\n",sims[i*HOODSIZE+j].userid,sims[i*HOODSIZE+j].sim);
    //        }
    //        }
    //        printf("begin write usersim\n");
    //        fwrite(sims,sizeof(UserSim),curidx,simFile);
    //        fclose(simFile);
}   

//获得计算两个人的相似度的
//皮尔森相关系数公式的分母（sigma i * sigma j)
float getSimBase(int i,int j){
    //如果sigma i或j=0，则返回0
    return users[i].sigma*users[j].sigma;
}

//计算两个用户的相似度
float getUsersSim(int l,int r){
    if(l==r) return 0;//同一个人跳过
    //        int s=l<r?l:r;int t=l<r?r:l;
    //        int i;//遍历相似度缓冲区
    //        for(i=s*HOODSIZE;i<(s+1)*HOODSIZE;i++){
    //            if(sims[i].userid<=0) break;
    //            if(sims[i].userid==t)
    //                return sims[i].sim;
    //        }
    int ldx=users[l].head;
    int rdx=users[r].head;
    float lavg=users[l].avgscore;
    float ravg=users[r].avgscore;
    float lsigma=users[l].sigma;
    float rsigma=users[r].sigma;
    float sim=0;
    while(true){
        if(ldx<0||rdx<0) break;
        if(Ratings[ldx].movieid==Ratings[rdx].movieid){
            float lscore=Ratings[ldx].score-lavg;
            float rscore=Ratings[rdx].score-ravg;
            sim+=(lscore*rscore);

            ldx=Ratings[ldx].next;
            rdx=Ratings[rdx].next;
        }
        while(rdx>=0&&Ratings[ldx].movieid>Ratings[rdx].movieid){
            rdx=Ratings[rdx].next;
        }
        while(ldx>=0&&Ratings[ldx].movieid<Ratings[rdx].movieid){
            ldx=Ratings[ldx].next;
        }
    }
    if(sim!=0){ sim=sim/(lsigma*rsigma); }//sims[i].userid=t;sims[i].sim=sim;}
    if(fabs(sim)>1) printf("ATTENTION:user %d and user %d has sim %f\n",l,r,sim);
    return sim;
}

void testData(){
    //测试获得的oldid字段是否正确完整
    printf("test:old id of %d is %d,should be 2649429\n",
            480189,users[480189].oldid);
    printf("test:old id of %d is %d,should be 2649376\n",
            480179,users[480179].oldid);

    printf("test users' head\n");
    int num=0;
    //测试Rating数据是否正确
    for(int i=1;i<=USERNUM;i++)
    {
        int head=users[i].head;
        int next=Ratings[head].next;
        while(next>=0){
            if(head>=next||Ratings[head].movieid>=Ratings[next].movieid){
                printf("ERROR:userid:%d head:%d next:%d\n",i,head,next);
                break;
            }
            //userid 字段
            else if(Ratings[head].userid!=i)
                printf("ERROR:rating %d userid %d while i:%d\n",head,Ratings[head].userid,i);
            num++;
            head=next;
            next=Ratings[next].next;
        }
        num++;
    }
    printf("visit %d ratings while RATINGNUM is %d\n",num,RATINGNUM);
}

//    //计算用户i和其他人的相似度,获得邻居(皮尔森相关系数)
//    //注意对于每个用户运算复杂度不能超过1000
//    void getUserSim(int l,map<int,float>& sims){
//
//        int i=users[l].head;
//        while(i>0)
//        {
//            Rating rate=Ratings[i];int score=rate.score;int movieid=rate.movieid;
//            float x=(float)score*RATIO-users[l].avgscore;
//            int head=movies[movieid].head;
//            int tail=movies[movieid+1].head;
//            int num=(int)(2*pow(movies[movieid].cnt,1.0/3));//该公式用来限制从电影中添加邻居的个数
//
//            for(int j=head;j<=head+num&&j<tail;j++){
//                int otherid=Ratings[j].userid;//其他对该电影评分的人 
//                int otherscore=Ratings[j].score;//其评分
//                float s=(x/users[l].sigma)*
//                    ((otherscore-users[otherid].avgscore)/users[otherid].sigma);
//                if(sims.find(otherid)!=sims.end()) {
//                    //                    float t=sims[otherid];
//                    sims[otherid]+=s;
//                } else sims[otherid]=s;
//            }
//            i=Ratings[i].next;//用户评论过的下一个电影
//        }
//    }

//    float getAdvice(int neighborid,int movieid){
//        int head=users[neighborid].head;
//        while(head>=0){
//            if(Ratings[head].movieid==movieid)
//                return Ratings[head].score-users[neighborid].avgscore;
//            if(Ratings[head].movieid>movieid) return 0;//该邻居没有对该电影评分，因此没有建议
//            head=Ratings[head].next;
//        }return 0;
//    }

//协同过滤
//计算用户对movie的预测评分
//找到看过电影的人，计算和他们的相似度
float predict(int userid,int movieid){
    //        int i=l*HOODSIZE;int j;
    //        float score=movies[m].avgscore;//电影获得的平均分
    //        int cnt=0;
    //        while((j=sims[i].userid)>0)
    //        {
    //            float advice=getAdvice(j,m);
    //            if(advice!=0) cnt++;
    //            score+=sims[i].sim*advice;//遍历邻居，获得建议(邻居的评分-邻居的平均评分)
    //            i++;
    //            if(i>=(l+1)*HOODSIZE) break;
    //        }
    //        if(cnt==0)
    //        {
    //            printf("\nmovie id:%d found none user\n",m);
    //            i=l*HOODSIZE;
    //            while(i<(l+1)*HOODSIZE){
    //                printf(" %d ",users[sims[i].userid].oldid);
    //                if(sims[i++].userid<=0) break;
    //            }
    //        }
    int cnt=0;float sim=0;
    float base_score=(movies[movieid].avgscore+users[userid].avgscore)/2;
    float ext_score=0;
    int head=movies[movieid].head;
    int tail=movies[movieid+1].head>head+400?head+400:movies[movieid+1].head;
    for(int j=head;j<tail;j++){
        int that_id=Ratings[j].userid;
        sim=getUsersSim(that_id,userid);
        if(sim!=0) {
            cnt++;
            //从that_id 对应用户寻求建议
            float that_score=(Ratings[j].score-users[that_id].avgscore);
            ext_score+=sim*that_score;
        }
        if(cnt>10) break;//只要十个人的建议即可
    }
    if(cnt==0)
        printf("no useful neighbour found from %d users\n",
                movies[movieid+1].head-movies[movieid].head);
    return cnt==0?base_score:base_score+(ext_score/cnt);
}

void getAnswer(){
    FILE *queryFile=fopen("datas/fmt_query.txt","r");
    if(queryFile==NULL) {
        printf("please run ./data2bin to generate fmt_query.txt\n");
        return ;
    }
    FILE *answerFile=fopen("predict.txt","w");
    char line[100]; char date[30]; int userid;int movieid=0;int qnum=0;
    while(fscanf(queryFile, "%s", line) != EOF) {
        if (line[strlen(line) - 1] == ':') {
            fprintf(answerFile,"%s\n",line);
            printf("%s\n",line);
            movieid = atoi(line);
        } else {
            sscanf(line, "%d,%s", &userid, date);
            fprintf(answerFile,"%.3lf\n", predict(userid,movieid));
//            qnum++;
//            if(qnum>10) break;
        }
    }
    fclose(queryFile);
    fclose(answerFile);
}
int main(){
    //    initUserMovieScoreLst(true);
    initData();
    initSim();
    testData();
    // testUserMovieLst(1);
    // testMovieUserLst(1);
    // printf("user:%d movie:%d finalscore:%f\n",1,1,predict(1,1));
    getAnswer();
    return 1;
}

