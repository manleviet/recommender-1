#include <cstdio>
#include <iostream>
#include <algorithm>
#include <queue>
#include <cmath>
#include <cstring>

#define eps 1e-8

using namespace std;

const int cnt_rating = 100480507, cnt_movie = 17770, cnt_user = 480189, sim_num = 30;

struct Rating {
    int movieid, userid, rating, next;
    void init(int _movieid, int _userid, int _rating) {
        movieid = _movieid, userid = _userid, rating = _rating;
    }
    bool operator <(const Rating &a) const {
        return movieid < a.movieid || movieid == a.movieid && userid < a.userid;
    }
}ratings[cnt_rating];

struct Movie {
    int begin, end, cnt;
    double rating, dat_rating;
    Movie () {
        begin = -1, cnt = 0, rating = 0, dat_rating = 0;
    }
}movie[cnt_movie + 1];

struct User {
    int begin, cnt;
    double rating, dat_rating;
    User () {
        begin = -1, cnt = 0, rating = 0, dat_rating = 0;
    }
}user[cnt_user + 1];

bool input() {
    FILE *fin, *fout;
    if ((fin = fopen("datas/rating.data", "r")) == NULL) {
        puts("ERROR: can't open file datas/rating.data");
        return false;
    }
    puts("reading data from rating.data");
    fread(ratings, sizeof(Rating), cnt_rating, fin);
    fclose(fin);

    puts("check first movie and first user");
    for(int i = cnt_rating - 1; i >= 0; --i) {
        int movieid = ratings[i].movieid;
        int userid = ratings[i].userid;
        int rating = ratings[i].rating;
        movie[movieid].begin = i;
        movie[movieid].rating += rating;
        ++ movie[movieid].cnt;

        user[userid].begin = i;
        user[userid].rating += rating;
        ++ user[userid].cnt;
    }

    puts("计算用户的平均打分");
    for(int i = 1; i <= cnt_user; ++i) {
        user[i].rating /= user[i].cnt;//
    }

    puts("计算电影的平均得分");
    for(int i = 1; i <= cnt_movie; ++i) {
        movie[i].rating /= movie[i].cnt;
        movie[i].end = movie[i].begin + movie[i].cnt - 1;
    }

    puts("计算用户的电影偏好");
    for (int i = 0; i < cnt_rating; ++i) {
        int movieid = ratings[i].movieid;
        int userid = ratings[i].userid;
        int rating = ratings[i].rating;
        movie[movieid].dat_rating += rating - user[userid].rating;
        user[userid].dat_rating += rating - movie[movieid].rating;
    }

    puts("计算用户的打分偏好");
    for(int i = 1; i <= cnt_user; ++i) {
        user[i].dat_rating /= user[i].cnt;
    }

    puts("计算电影的用户偏好");
    for(int i = 1; i <= cnt_movie; ++i) {
        movie[i].dat_rating /= movie[i].cnt;
    }

    if ((fout = fopen("datas/movie.txt", "w")) != NULL) {
        for(int i = 1; i <= cnt_movie; ++i) {
            fprintf(fout, "%d %d %.3lf %.3lf\n", i, movie[i].cnt, movie[i].rating, movie[i].dat_rating);
        }
        fclose(fout);
    }

    if ((fout = fopen("datas/user.txt", "w")) != NULL) {
        for(int i = 1; i <= cnt_user; ++i) {
            fprintf(fout, "%d %d %.3lf %.3lf\n", i, user[i].cnt, user[i].rating, user[i].dat_rating);
        }
        fclose(fout);
    }
    return true;
}

double predict(int movieid, int userid) {
    double rating1 = min(max(user[userid].rating + movie[movieid].dat_rating, 1.0), 5.0);
    double rating2 = min(max(movie[movieid].rating + user[userid].dat_rating, 1.0), 5.0);
    int threshold = 30;
    double lamdba = 1.0 * min(user[userid].cnt, threshold) / (min(user[userid].cnt, threshold) + min(movie[movieid].cnt, threshold));
    return lamdba * rating1 + (1 - lamdba) * rating2;
}

bool output() {
    FILE *fin, *fout;
    if ((fin = fopen("datas/fmt_query.txt", "r")) == NULL) {
        puts("ERROR: can't open file datas/fmt_query.data");
        return false;
    }

    if ((fout = fopen("datas/answer.txt", "w")) == NULL) {
        printf("ERROR: can't open datas/answer.txt");
        return false;
    }

    char line[100], date[100];
    int userid, movieid;
    while(fscanf(fin, "%s", line) != EOF) {
        if (line[strlen(line) - 1] == ':') {
            fprintf(fout, "%s\n", line);
            movieid = atoi(line);
            //printf("now movie id is %d\n", movieid);
        } else {
            sscanf(line, "%d,%s", &userid, date);
            fprintf(fout, "%.3lf\n", predict(movieid, userid));
        }
    }
    fclose(fin);
    fclose(fout);
}

int cntbits(int x) {
    int bits = 0;
    while(x) {
        ++bits;
        x /= 10;
    }
    return bits;
}

int main(int argc, char *argv[]) {
    if (argc != 2) {
        puts("Useage: ./forcast file.out");
        return 1;
    }
    input();
    output();
    return 0;
}

