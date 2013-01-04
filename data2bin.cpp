#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <errno.h>
#include <map>
#include <set>
#include <algorithm>
#include <cstring>

using namespace std;
#ifndef USERNUM
#define USERNUM 480190
#endif
#ifndef MOVIENUM
#define MOVIENUM 17780
#endif
//数据预处理
//变量命名：容器采用容器名作前缀。如map_userid
struct Rating {
    int movieid, userid, rating, next;
    void init(int _movieid, int _userid, int _rating) {
        movieid = _movieid, userid = _userid, rating = _rating;
    }
    bool operator <(const Rating &a) const {
        return movieid < a.movieid || movieid == a.movieid && userid < a.userid;
    }
}Ratings[100480507];

map<int, int> map_userid;
int cnt_rating = 0, arr_user_head[480190];
bool input(char dirname[]) {
    DIR *dp;
    FILE *fin;
    struct dirent *dirp;
    if ((dp = opendir(dirname)) == NULL) {
        printf("ERROR: can't open dir %s\n", dirname);
        return false;
    }

    set<int> set_userid;
    set<int>::iterator it;
    int cnt_movie = 0;
    while ((dirp = readdir(dp)) != NULL) {
        char filename[100];
        ++ cnt_movie;
        sprintf(filename, "%s%s", dirname, dirp->d_name);
        printf("%s\n",filename);
        if((fin = fopen(filename, "r")) == NULL) {
            printf("ERROR: can't open file %s\n", dirp->d_name);
        } else {
            int movieid, userid, rating;
            char date[20];
            fscanf(fin, "%d:", &movieid);
            while(fscanf(fin, "%d,%d,%s", &userid, &rating, date) != EOF) {
                set_userid.insert(userid);
                Ratings[cnt_rating++].init(movieid, userid, rating);
            }
            fclose(fin);
        }
    }

    int i = 0;
    for(it = set_userid.begin(); it != set_userid.end(); ++it) {
        map_userid[*it] = ++i;//从1开始对用户重新编号
        printf("oldid:%d newid:%d\n",*it,i);
    }

    for(int i = cnt_rating - 1; i >= 0; --i) {
        //把用户id改成连续编号
        Ratings[i].userid = map_userid[Ratings[i].userid];
    }

    printf("total %d movies, %d users, %d Ratings\n", cnt_movie, map_userid.size(), cnt_rating);
    return true;
}

bool output(char *queryfile) {
    FILE *fin, *fout;

    puts("begin output rating by movie");
    //按照电影编号对Ratings排序
    sort(Ratings, Ratings + cnt_rating);
    //同一个用户的记录之间用next连接起来
    memset(arr_user_head, -1, sizeof(arr_user_head));
    for(int i = cnt_rating - 1; i >= 0; --i) {
        Ratings[i].next = arr_user_head[Ratings[i].userid];
        arr_user_head[Ratings[i].userid] = i;
    }
    if((fout = fopen("datas/rating.data", "wb")) == NULL) {
        puts("ERROR: can't open file rating.data");
        return false;
    }
    fwrite(Ratings, sizeof(Rating), cnt_rating, fout);
    fclose(fout);

    if((fout = fopen("datas/user_ratings_head.data", "wb")) == NULL) {
        puts("ERROR: can't open file rating.data");
        return false;
    }
    fwrite(arr_user_head, sizeof(int), USERNUM, fout);
    fclose(fout);

    //将输入query文件格式化输出（修改userid为连续编号以后的userid）
    if((fin = fopen(queryfile, "r")) == NULL) {
        printf("ERROR: can't open queryfile %s\n", queryfile);
        return false;
    }
    if((fout = fopen("datas/fmt_query.txt", "w")) == NULL) {
        puts("ERROR: can't open fmt_query.txt");
        return false;
    }
    //输入query文件格式：  movieid:\nuserid,data\n
    char line[100], date[100];
    int userid;
    while(fscanf(fin, "%s", line) != EOF) 
    {
        if(line[strlen(line) - 1] == ':') {
            fprintf(fout, "%s\n", line);
        } 
        else {
            sscanf(line, "%d,%s", &userid, date);
            if (map_userid.find(userid) != map_userid.end()) {
                fprintf(fout, "%d,%s\n", map_userid[userid], date);//修改userid
            } 
            else {
                fprintf(fout, "0,%s\n", date);
                printf("userid:%d not found\n",userid);
            }
        }
    }
    fclose(fin);
    fclose(fout);
}

int main(int argc,char *argv[])
{
    if (argc != 3) {
        puts("Usage: datatobin input.dir query.file");
        return 1;
    }
    input(argv[1]);
    output(argv[2]);
    return 0;
}

