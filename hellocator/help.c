#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h>
#include <unistd.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#include "hellocator.h"
#define min(a, b) ((a) < (b) ? (a) : (b))

int sqlite_init()
{
    sqlite3 *db;
    char *err = 0;
    char *sql;
    int  res;

    res = sqlite3_open("db.db", &db);
    if (res) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return 1;
    } else {
        fprintf(stdout, "Opened database successfully\n");
    }

    sql = "CREATE TABLE IF NOT EXISTS user("  \
          "ID INTEGER PRIMARY KEY AUTOINCREMENT         NOT NULL," \
          "NAME           TEXT        NOT NULL," \
          "PASSWORD       CHAR(10)    NOT NULL," \
          "SUM            TEXT        NOT NULL);";

    res = sqlite3_exec(db, sql, 0, 0, &err);
    if (res != SQLITE_OK ) {
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
    const char *user_dir = USERS_DIR;
    char *curdir = 0;
    struct stat statbuf;

    if (stat(user_dir, &statbuf) == -1) {
        if (errno != ENOENT) {
            perror("stat USERS_DIR error");
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
                perror("Can't create USERS_DIR directory");
                return 1;
            } else {
                fprintf(stdout, "Created USERS_DIR directory\n");
            }
            free(curdir);
        }
    } else if (!S_ISDIR(statbuf.st_mode)) {
        perror("USERS_DIR is not directory");
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

static int callback_auth(void *result, int argc, char **argv, char **azColName) {
    if (argc > 0) {
        strcpy((char *)result, argv[0]);
    } else {
        strcpy((char *)result, "error");
    }
    return 0;

}

/*
char *hellhash(const char *login, const char *pass) {
    uint32_t a = *(uint32_t *)login;
    uint32_t b = *(uint32_t *)pass;
    a = a ^ b;
    char *sum = malloc(8*sizeof(char));
    sprintf(sum, "%02x", a);
    return sum;
}
*/

int base64encode(const void* data_buf, size_t dataLength, char* result, size_t resultSize)
{
   const char base64chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
   const uint8_t *data = (const uint8_t *)data_buf;
   size_t resultIndex = 0;
   size_t x;
   uint32_t n = 0;
   int padCount = dataLength % 3;
   uint8_t n0, n1, n2, n3;

   /* increment over the length of the string, three characters at a time */
   for (x = 0; x < dataLength; x += 3) 
   {
      /* these three 8-bit (ASCII) characters become one 24-bit number */
      n = ((uint32_t)data[x]) << 16; //parenthesis needed, compiler depending on flags can do the shifting before conversion to uint32_t, resulting to 0
      
      if((x+1) < dataLength)
         n += ((uint32_t)data[x+1]) << 8;//parenthesis needed, compiler depending on flags can do the shifting before conversion to uint32_t, resulting to 0
      
      if((x+2) < dataLength)
         n += data[x+2];

      /* this 24-bit number gets separated into four 6-bit numbers */
      n0 = (uint8_t)(n >> 18) & 63;
      n1 = (uint8_t)(n >> 12) & 63;
      n2 = (uint8_t)(n >> 6) & 63;
      n3 = (uint8_t)n & 63;
            
      /*
       * if we have one byte available, then its encoding is spread
       * out over two characters
       */
      if(resultIndex >= resultSize) return 1;   /* indicate failure: buffer too small */
      result[resultIndex++] = base64chars[n0];
      if(resultIndex >= resultSize) return 1;   /* indicate failure: buffer too small */
      result[resultIndex++] = base64chars[n1];

      /*
       * if we have only two bytes available, then their encoding is
       * spread out over three chars
       */
      if((x+1) < dataLength)
      {
         if(resultIndex >= resultSize) return 1;   /* indicate failure: buffer too small */
         result[resultIndex++] = base64chars[n2];
      }

      /*
       * if we have all three bytes available, then their encoding is spread
       * out over four characters
       */
      if((x+2) < dataLength)
      {
         if(resultIndex >= resultSize) return 1;   /* indicate failure: buffer too small */
         result[resultIndex++] = base64chars[n3];
      }
   }  

   /*
    * create and add padding that is required if we did not have a multiple of 3
    * number of characters available
    */
   if (padCount > 0) 
   { 
      for (; padCount < 3; padCount++) 
      { 
         if(resultIndex >= resultSize) return 1;   /* indicate failure: buffer too small */
         result[resultIndex++] = '=';
      } 
   }
   if(resultIndex >= resultSize) return 1;   /* indicate failure: buffer too small */
   result[resultIndex] = 0;
   return 0;   /* indicate success */
}

char *hellhash(const char *login, const char *pass) {
    size_t log_len = strlen(login);
    size_t pass_len = strlen(pass);
    size_t hash_len = min(log_len, pass_len);
    char *hash = (char *)malloc(hash_len);
    int i;

    for (i = 0; i < hash_len; i++) {
        hash[i] = login[i]/pass[i];
    }

    size_t hash_result_size = (hash_len/3)*4 + 10;
    char *hash_result = (char *)malloc(hash_result_size);
    base64encode(hash, hash_len, hash_result, hash_result_size);

    free(hash);
    return hash_result;
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
    if (res) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return 1;
    } else {
        fprintf(stdout, "auth_step: Open database!\n");
    }

    char *sum=hellhash(login, password);
    char *sqlr = calloc(400, sizeof(char));
    char *sql = "SELECT name FROM user WHERE (name='";
    strcat(sqlr, sql);
    strcat(sqlr, login);
    strcat(sqlr, "' and sum='");
    strcat(sqlr, sum);
    strcat(sqlr, "');");

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
