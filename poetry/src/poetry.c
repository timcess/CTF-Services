#include <stdio_ext.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdarg.h>
#include <string.h>
#include <dirent.h>
#include <stdarg.h>
#include <dirent.h>
#include <stdio.h>
#include <errno.h>
#include <stdio.h>

#include "database.h"

extern const char *RSA;

typedef enum {
	WRONG_COMMAND,
	LOGIN,
	REGISTER,
	POST_POEM,
	LEVEL_UP,
	LOOK_AT_OTHER_USER,
	READ_POEM,
	EXIT
} ClientCommand;

struct GlobalEnv {
	int is_login;
	User *user;
};

typedef struct GlobalEnv GlobalEnv;

GlobalEnv Env;

void say (const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vprintf(fmt, args);
	va_end(args);

	fflush(stdout);
}

void listen (const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	vscanf(fmt, args);
	va_end(args);

	__fpurge(stdin);
	rewind(stdin);
}

static void say_hello() {
	say("\n");
	say("%s\n", WELCOME);
	say("            Just type the command number             \n");
	say("-----------------------------------------------------\n");
}

static void say_menu ()
{
	say("\n");
	say("1. Login\n");
	say("2. Register\n");
	say("3. Post poem\n");
	say("4. Level up\n");
	say("5. Look at other user\n");
	say("6. Read poem\n");
	say("7. Exit\n");
}

static ClientCommand convert_to_ClientCommand (const char *char_command)
{
	if (!char_command) {
		return WRONG_COMMAND;
	}

	int x = char_command[0] - '0';
	if (x == LOGIN) {
		return LOGIN;
	}
	if (x == REGISTER) {
		return REGISTER;
	}
	if (x == POST_POEM) {
		return POST_POEM;
	}
	if (x == LEVEL_UP) {
		return LEVEL_UP;
	}
	if (x == LOOK_AT_OTHER_USER) {
		return LOOK_AT_OTHER_USER;
	}
	if (x == READ_POEM) {
		return READ_POEM;
	}
	if (x == EXIT) {
		return EXIT;
	}

	return WRONG_COMMAND;
}

int check_login(const char *login)
{
	int i;
	for (i = 0; login[i]; i++) {

		if (login[i] == '_')
			continue;
		if (login[i] >= 'a' && login[i] <= 'z')
			continue;
		if (login[i] >= 'A' && login[i] <= 'Z')
			continue;
		if (login[i] >= '0' && login[i] <= '9')
			continue;

		return 1;

	}

	return 0;

}

void login_handler()
{
	char *login = malloc(MAX_LOGIN_LEN);
	char *password = malloc(MAX_PASSWORD_LEN);
	User *user;
	int ret;
	char format[10];

	say("\n");
	say("Enter login: ");
	sprintf(format, "%%%ds", MAX_LOGIN_LEN);
	listen(format, login);

	say("Enter password: ");
	sprintf(format, "%%%ds", MAX_PASSWORD_LEN);
	listen(format, password);

	if (check_login(login) != 0) {
		say("Invalid login\n");
		return;
	}

	ret = find_user(login, &user);
	if (ret == -1) {
		say("System failure\n");
		return;
	}
	if (ret == -3) {
		say("Wrong login\n");
		return;
	}

	if (user == NULL) {
		say("Failed login. Strange case.\n");
	} else {
		if (strcmp(password, user->password) != 0) {
			printf("Invalid password\n");
		} else {
			user->login = login;
			Env.is_login = 1;
			Env.user = user;
			printf("Logged as %s\n", user->login);
		}
	}
}

void register_handler()
{
	char *login = malloc(MAX_LOGIN_LEN);
	char *password = malloc(MAX_PASSWORD_LEN);
	User *user;
	int ret;
	char format[10];

	say("\n");
	say("Enter login: ");
	sprintf(format, "%%%ds", MAX_LOGIN_LEN);
	listen(format, login);

	say("Enter password: ");
	sprintf(format, "%%%ds", MAX_PASSWORD_LEN);
	listen(format, password);

	if (check_login(login) != 0) {
		say("Invalid login\n");
		return;
	}

	ret = find_user(login, &user);

	if (ret == 0) {
		say("Name is already in use\n");
	}

	if (ret == -3) {
		user->login = login;
		user->password = password;

		//determine level
		unsigned long long number, number2;
		char *encrypted_number = calloc(2048, sizeof(char));
		int ret;
		char command[1024];
		sprintf(command, "echo -n '%s' | base64 -d | python > ct", RSA);
		system(command);
		FILE *ct = fopen("./ct", "r");
		fscanf(ct, "%llu", &number);
		ret = fread(encrypted_number, sizeof(char), 1024, ct);
		fclose(ct);

		if (ret != 0) {
			/* DEBUG
			printf("%llu\n", number);
			*/

			say("Hmm, by the way, can you understand what is written there?");
			say("%s\n", encrypted_number);
			listen("%llu", &number2);
			if (number != number2) {
				say("Wrong answer. Never mind.\n");
				user->level = 0;
			} else {
				say("Come in, my lord\n");
				user->level = 1;
			}
		}
		dump_user_struct(user);
	}

	say("You have been successfully registered\n");
}

void post_poem_handler()
{

	if (Env.user == NULL) {
		say("Login at first\n");
		return;
	}

	say("Post your poem:\n");
	say("Remember, no more than %d lines!\n", STRING_COUNT);
	User *user = Env.user;
	char *next_line = user->poem;
	char format[50];
	sprintf(format, " %%%d[^\n]", STRING_LENGTH);
	int len = 0;
	int line = 0;
	memset(user->poem, '\0', MAX_POEM_LEN);
	say(">");
	do {
		listen(format, next_line);
		len = strlen(next_line);
		next_line[len] = '\n';
		len = strlen(next_line);
		next_line += len;
		line++;
	} while (len > 0 && line < STRING_COUNT);

	dump_user_struct(user);
	say("Your poem is successfully posted");
}

void level_up_handler()
{
	if (Env.user == NULL) {
		say("Login at first\n");
		return;
	}

	say("Which user do you want to level up?\n");
	say(">");

	/*level up user =)*/
	User *luuser;
	char luuser_login[MAX_LOGIN_LEN];
	char format[50];
	sprintf(format, "%%%ds", MAX_LOGIN_LEN);
	listen(format, luuser_login);

	int ret;
	ret = find_user(luuser_login, &luuser);
	if (ret == -1) {
		say("System failure\n");
		return;
	}
	if (ret == -3) {
		say("Wrong login\n");
		return;
	}

	luuser->login = luuser_login;
	if (Env.user->level > luuser->level) {
		luuser->level = Env.user->level;
		dump_user_struct(luuser);
		say("Level upped!\n");
	} else {
		say("Your level is too low\n");
	}
}

void look_at_handler()
{
	if (Env.user == NULL) {
		say("Login at first\n");
		return;
	}

	say("Which user do you want to look at?\n");
	say(">");

	User *lauser;
	char lauser_login[MAX_LOGIN_LEN];
	char format[50];
	sprintf(format, "%%%ds", MAX_LOGIN_LEN);
	listen(format, lauser_login);

	int ret;
	ret = find_user(lauser_login, &lauser);
	if (ret == -1) {
		say("System failure\n");
		return;
	}
	if (ret == -3) {
		say("Wrong login\n");
		return;
	}
	lauser->login = lauser_login;

	if (Env.user->level > lauser->level) {
		say("%s poem\n", lauser->login);
		say("------------------------\n");
		say("%s", lauser->poem);
		say("The end\n");
	} else {
		say("Your level is too low\n");
	}

}

void read_poem_handler() {
	if (Env.user == NULL) {
		say("Login at first\n");
		return;
	}

	say("Your poem\n");
	User *user = Env.user;
	if (strlen(user->poem) != 0) {
		say("%s", user->poem);
	}
}

void main_handler()
{
	ClientCommand command;
	const char client_input[10];
	User *user = calloc(1, sizeof(user));
	say_hello();

	while (1) {
		say_menu();
		say("> ");
		listen("  %c", client_input);
		command = convert_to_ClientCommand(client_input);

		switch (command){
			case LOGIN:
			login_handler();
			break;
		case REGISTER:
			register_handler();
			break;
		case POST_POEM:
			post_poem_handler();
			break;
		case LEVEL_UP:
			level_up_handler();
			break;
		case LOOK_AT_OTHER_USER:
			look_at_handler();
			break;
		case READ_POEM:
			read_poem_handler();
			break;
		case EXIT:
			say("Goodbye!\n");
			return;
		default:
			say("WRONG command, try another!\n");
			break;
		}
	}
}

int main ()
{
	init_pool();

	int check;
	check = mkdir(DATABASE_PATH, S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH);
	if (check != 0) {
		if (errno != EEXIST) {
			perror("Failed create database\n");
			return 1;
		}
	}

	Env.user = NULL;
	Env.is_login = 0;

	main_handler();

	free_pool();
	return 0;
}
