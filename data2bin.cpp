#include <sys/types.h>
#include <dirent.h>
#include <stdio.h>
#include <errno.h>
#include <map>
#include <set>
#include <algorithm>
#include <cstring>

using namespace std;

struct Rating {
    int movieid, userid, rating, next;
    void init(int _movieid, int _userid, int _rating) {
        movieid = _movieid, userid = _userid, rating = _rating;
    }
    bool operator <(const Rating &a) const {
        return movieid < a.movieid || movieid == a.movieid && userid < a.userid;
    }
}ratings[100480507];

map<int, int> map_useid;
int cnt_rating = 0, hash_user[480190];

bool input(char dirname[]) {
    DIR *dp;
    FILE *fin;
    struct dirent *dirp;
    if ((dp = opendir(dirname)) == NULL) {
        printf("ERROR: can't open dir %s\n", dirname);
        return false;
    }

    set<int> set_useid;
    set<int>::iterator it;
    int cnt_movie = 0;
    while ((dirp = readdir(dp)) != NULL) {
        char filename[100];
        ++ cnt_movie;
        sprinte(filename, "%s%s", dirname, dirp->d_name);
                printf("%s\n",filename);
        if((fin = fopen(filename, "r")) == NULL) {
            printf("ERROR: can't open file %s\n", dirp->d_name);
        } else {
            int movieid, userid, rating;
            char date[20];
            fscanf(fin, "%d:", &movieid);
            while(fscanf(fin, "%d,%d,%s", &userid, &rating, date) != EOF) {
                set_useid.insert(userid);
                ratings[cnt_rating++].init(movieid, userid, rating);
            }
            fclose(fin);
        }
    }

    int i = 0;
    for(it = set_useid.begin(); it != set_useid.end(); ++it) {
        map_useid[*it] = ++i;
    }

    memset(hash_user, -1, sizeof(hash_user));

    for(int i = cnt_rating - 1; i >= 0; --i) {
        ratings[i].userid = map_useid[ratings[i].userid];
        ratings[i].next = hash_user[ratings[i].userid];
        hash_user[ratings[i].userid] = i;
    }

    printf("total %d movies, %d users, %d ratings\n", cnt_movie, map_useid.size(), cnt_rating);
    return true;
}

bool output(char *queryfile) {
    FILE *fin, *fout;

    puts("begin output rating by movie");
    sort(ratings, ratings + cnt_rating);
    if((fout = fopen("datas/rating.data", "wb")) == NULL) {
        puts("ERROR: can't open file rating.data");
        return false;
    }
    fwrite(ratings, sizeof(Rating), cnt_rating, fout);
    fclose(fout);

    puts("begin output fmt_query file");
    if((fin = fopen(queryfile, "r")) == NULL) {
        printf("ERROR: can't open queryfile %s\n", queryfile);
        return false;
    }
    if((fout = fopen("datas/fmt_query.txt", "w")) == NULL) {
        puts("ERROR: can't open fmt_query.txt");
        return false;
    }

    char line[100], date[100];
    int userid;
    while(fscanf(fin, "%s", line) != EOF) {
        if(line[strlen(line) - 1] == ':') {
            fprintf(fout, "%s\n", line);
        } else {
            sscanf(line, "%d,%s", &userid, date);
            if (map_useid.find(userid) != map_useid.end()) {
                fprintf(fout, "%d,%s\n", map_useid[userid], date);
            } else {
                fprintf(fout, "0,%s\n", date);
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

