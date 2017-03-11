#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

int sqlite_init()
{
   sqlite3 *db;
   char *err = 0;
   char *sql;
   int  res;

   res = sqlite3_open("db.db", &db);
   if(res){
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      return 1;
   }else{
      fprintf(stdout, "Opened database successfully\n");
   }

   sql = "CREATE TABLE IF NOT EXISTS user("  \
         "ID INTEGER PRIMARY KEY AUTOINCREMENT         NOT NULL," \
         "NAME           TEXT        NOT NULL," \
         "PASSWORD       CHAR(10)    NOT NULL," \
         "SUM            CHAR(16)    NOT NULL);";

   res = sqlite3_exec(db, sql, 0, 0, &err);
   if(res != SQLITE_OK ){
       fprintf(stderr, "SQL error: %s\n", err);
       sqlite3_free(err);
       return 1;
   } else {
      fprintf(stdout, "Table created successfully\n");
   }
   sqlite3_close(db);

   return 0;
}

int create_users_database(const char *login)
{
    sqlite3 *db;
    int retval;
    char *sql, *err = 0;
    const char *user_dir = "usersSpaces";
    char *curdir = 0;
    struct stat statbuf;

    if (stat(user_dir, &statbuf) == -1) {
        if (errno != ENOENT) {
            perror("stat(usersSpaces) error");
            return 1;
        } else {
            curdir = getcwd(curdir, 0);
            if (curdir == 0) {
                perror("Can't get current directory name");
            }
            if (stat(curdir, &statbuf) == -1) {
                perror("stat(current_directory) error");
                return 1;
            }
            if (mkdir(user_dir, statbuf.st_mode) == -1) {
                printf("%s", curdir);
                perror("Can't create usersSpaces directory");
                return 1;
            } else {
                fprintf(stdout, "Created usersSpaces directory\n");
            }
            free(curdir);
        }
    } else if (!S_ISDIR(statbuf.st_mode)) {
        perror("usersSpaces is not directory");
        return 1;
    } 

    chdir (user_dir);
    retval = sqlite3_open(login, &db);
    if (retval) {
        fprintf(stderr, "Can't open user's database: %s\n", sqlite3_errmsg(db));
        return 1;
    }

    fprintf(stdout, "Created a new database for user %s\n", login);

    sql = "CREATE TABLE IF NOT EXISTS spaceinfo(" \
          "FREESPACE BIGINTEGER DEFAULT NULL,"     \
          "OCCUPIEDSPACE BIGINTEGER DEFAULT NULL);" ;

    retval = sqlite3_exec(db, sql, 0, 0, &err);
    if (retval != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", err);
        sqlite3_free(err);
        return 1;
    }

    sql = "INSERT INTO spaceinfo VALUES (0, 0);";
    retval = sqlite3_exec(db, sql, 0, 0, &err);
    if (retval != SQLITE_OK)
    {
        fprintf(stderr, "SQL error: %s\n", err);
        sqlite3_free(err);
        return 1;
    }
    fprintf(stdout, "Table with information for user %s created successfully\n", login);

    sql = "CREATE TABLE IF NOT EXISTS files(" \
          "FILENAME TEXT PRIMARY KEY);"; 

    retval = sqlite3_exec(db, sql, 0, 0, &err);
    if (retval != SQLITE_OK) {
        fprintf(stderr, "SQL error:%s\n", err);
        sqlite3_free(err);
        return 1;
    }

    sqlite3_close(db);
    chdir("..");
    return 0;
}

static int callback_auth(void *result, int argc, char **argv, char **azColName){
        if (argc > 0) {
            strcpy((char *)result, argv[0]);
        } else {
            strcpy((char *)result, "error");
        }
        return 0;

}

char *hellhash(const char *login, const char *pass) {
    uint32_t a = *(uint32_t *)login;
    uint32_t b = *(uint32_t *)pass;
    a = a ^ b;
    char *sum = malloc(8*sizeof(char));
    sprintf(sum, "%02x", a);
    return sum;
}

int
check_step_second(char *login, char *password) {
    if (strstr(login, "'")) {
        return 0;
    } else if (strstr(login, "\"")) {
        return 0;
    } else if (strstr(login, "%")) {
        return 0;
    } else if (strstr(password, "'")) {
        return 0;
    } else if (strstr(password, "\"")) {
        return 0;
    } else if (strstr(password, "%")) {
        return 0;
    }
    return 1;
}

int auth_step(char *login, char *password, char *result) {
   sqlite3 *db;
   char *err = 0;
   int  res;
   sqlite3_stmt *stmt;

   res = sqlite3_open("db.db", &db);
   if(res) {
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      return 1;
   } else {
      fprintf(stdout, "auth_step: Open database!\n");
   }

   char *sum=hellhash(login, password);
   char *sqlr = calloc(200, sizeof(char));
   char *sql = "SELECT name FROM user WHERE name='";
   strcat(sqlr, sql);
   strcat(sqlr, login);
   strcat(sqlr, "' and password='");
   strcat(sqlr, password);
   strcat(sqlr, "' or sum='");
   strcat(sqlr, sum);
   strcat(sqlr, "';");

   sqlite3_exec(db, sqlr, callback_auth, result, &err);
   if (strstr(result, "error")) {
       strcpy(result, "Login failed: either login or password is wrong\n");
       return 1;
   } else if (strcmp(result, login) != 0) {
       strcpy(result, "Login failed: either login or password is wrong\n");
       return 1;
   }

   free(sum);
   return 0;
}
