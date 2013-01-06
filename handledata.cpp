#include <stdio.h>
#include <queue>
#include <map>
#include <algorithm>
#include <unistd.h>
#include <math.h>
#include <cmath>
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
	float pref_score;//计算偏好
	float avgscore;//平均评分
	float sigma;//皮尔森系数的分母
}movies[MOVIENUM + 2];

struct User {//某个用户的属性
	int oldid;//保存旧的编号
	int head, cnt;
	float pref_score;
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

typedef pair<int,float> PAIR;  
class cmp{
	public : int operator()(const PAIR& x, const PAIR& y)  {  return x.second < y.second;  }  
};

void getUserSim(int l,map<int,float> &sims);

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
		movies[MOVIENUM+1].head=RATINGNUM;//tail tag
	
//		if(access("datas/user_mapback.data",0)!=-1){ printf("reading user_mapback.data\n");
//			int user_mapback[USERNUM+1];
//			FILE *fin=fopen("datas/user_mapback.data","r");
//			fread(user_mapback,sizeof(int),USERNUM+1,fin);
//			fclose(fin);
//			printf("read user_mapback.data successfully\n");
//			for(int i=1;i<=USERNUM;i++)
//				users[i].oldid=user_mapback[i];
//		}else {
//			printf("ERROR:cannot open user_mapback.data for read,\
//					please run data2bin to generate it!\n");
//			return ;
//		}

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

		printf("user avg score and sigma:\n");
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

		//计算偏好
		for(int i=0;i<RATINGNUM;i++){
			int movieid = Ratings[i].movieid;
			int userid = Ratings[i].userid;
			int score = Ratings[i].score;
			movies[movieid].pref_score += score - users[userid].avgscore;
			users[userid].pref_score += score - movies[movieid].avgscore;
		}

		//计算用户的打分偏好
		for(int i=1;i<=USERNUM;i++){
			users[i].pref_score /= users[i].cnt;
		}

		//计算电影的用户偏好
		for(int i=1;i<=MOVIENUM;i++){
			movies[i].pref_score /= movies[i].cnt;
		}

		FILE *fout;
		//输出users、movies
		if(sorted){
			printf("output users to users.data\n");
			fout=fopen("datas/users.data","wb"); 
			fwrite(users,sizeof(User),USERNUM+1,fout);
			fclose(fout);
		}
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
		printf("output users to user.data\n");
		FILE* usersDataFile=fopen("datas/users.data","wb");
		fwrite(users,sizeof(User),USERNUM+1,usersDataFile);
		fclose(usersDataFile);

		printf("output sortedratings to sortedrating.data\n");
		FILE *sortedRatingFile = fopen("datas/sortedrating.data", "wb");
		fwrite(Ratings,sizeof(Rating),RATINGNUM,sortedRatingFile);
		fclose(sortedRatingFile);
	}
	printf("all data has been initialized!\n");
}

//计算用户之间的相似度,获得邻居
//并保存到sim.data
void initSim(){
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

//协同过滤
//计算用户对movie的预测评分
//找到看过电影的人，计算和他们的相似度
float predict(int userid,int movieid){
	int cnt=0;float sim=0;
	float base_score=0.;
	//float base_score=(movies[movieid].avgscore+users[userid].avgscore)/2;
	float rating1=min(max(users[userid].avgscore+movies[movieid].pref_score, (float)1.), (float)5.);
	float rating2=min(max(movies[movieid].avgscore+users[userid].pref_score, (float)1.), (float)5.);
	int threshold=15;
	float lamdba=1.*min(users[userid].cnt, threshold)/(min(users[userid].cnt, threshold)+min(movies[movieid].cnt, threshold));
	base_score=lamdba*rating1+(1-lamdba)*rating2;

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

