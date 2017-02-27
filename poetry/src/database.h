#include <sys/mman.h>
#include <stdlib.h>
#include <stdio.h>

#define WELCOME "*****************************************************\n*****************HELLO, POEM WRITER!*****************\n*****************************************************"

#define DATABASE_PATH "./database"
#define STRING_LENGTH 35
#define STRING_COUNT 25
#define MAX_LOGIN_LEN 20
#define MAX_PASSWORD_LEN 20
#define MAX_POEM_LEN STRING_COUNT*STRING_LENGTH
#define POOL_START_SIZE 1024


/*Flags will be in poems*/
struct User {
	char *login;
	char *password;
	int level;
	/*
		Poem is limited by STRING_COUNT lines. Line is a string without \n.
		Line must be not more than STRING_LENGTH length.
	*/
	char poem[STRING_LENGTH*STRING_COUNT];
};

typedef struct User User;

User *user_pool;
int users_allocated;

int find_user(const char *login, User **user);

/*Write user struct in user file*/
int dump_user_struct(User * user);
int post_poem(User *user);
void init_pool();
void free_pool();
User *user_allocator();
