#include <string.h>
#include <errno.h>

#include "database.h"

const char *RSA = "aW1wb3J0IHJhbmRvbQpudW1iZXIgPSByYW5kb20ucmFuZGludCgzLCAoMioqNjQpLTEpCgpuID0gOTM4OTQ1OTgxNjI4NzU3OTgxNzU4Mjc5NzkzNjg4MjU0NDQ0NzA0MTM2MDkzNTU0OTcyMTY2Mzg0ODIxMDI1ODgzMzcyMjIxMDA5MDU5ODMzODQ4MzMzODc2NDY3NzQ2MzQ0NTM5NTczMDAwODkyNDcxMDM2NzQ5MjE1ODcwNjY2MDg5NjAwODk4MDA4ODYzNDQ3MTc3NzQ5MDIzNzUxODM1MDQwOTIyMzA5MjgxNDk1MjcyNjM0MTIzNDMwMzMxNDI0NjM5NzEyMzM3Nzc0NDIwNzU4MDU2NzA3MzA2MDY5NDI1NDk2MTQ5Njk5ODE3MjAwODAxNDgyMjUwODk4ODkyMzA0OTEyMjc4ODU1OTc0ODc3NjgyOTg4NjQ0OTI5Nzg4Mjg5MDQ5MTI0NjM0MTYwMzMKZSA9IDY1NTM3CnkgPSBwb3cobnVtYmVyLCBlLCBuKQoKcHJpbnQgbnVtYmVyCnByaW50IGhleCh5KQo=";

FILE * open_user_directory(const char *login, const char *mode) {
	char path[50];
	FILE *file;

	sprintf(path, "%s/%s.txt", DATABASE_PATH, login);

	/* DEBUG
	printf("\x1b[35m[D] PATH: %s \x1b[0m\n", path);
	*/

	file = fopen(path, mode);
	if (file == NULL && errno != ENOENT) {
			perror("Failed open user file: ");
			fflush(stderr);
			return NULL;
	}

	return file;
}

int find_user(const char *login, User **user_to_fill)
{
	if (login == NULL) {
		printf("Corrupted login\n");
		fflush(stdout);
		return -1;
	}

	User *user = user_allocator();
	if (user == NULL) {
		printf("Failed to allocate memory for user\n");
		fflush(stdout);
		return -1;
	}
	*user_to_fill = user;

	FILE *file = open_user_directory(login, "r");
	if (file == NULL && errno == ENOENT) {
		return -3;
	}

	char string[80];
	char key[20];
	char value[50];
	int level;
	int i;
	char *ret;

	for (i = 0; i < 3; i++) {

		ret = fgets(string, sizeof(string), file);
		if (ret == NULL) {
			printf("Corrupted file");
			return -1;
		}

		sscanf(string, "%s %s", key, value);

		if (strcmp(key, "password") == 0)
			user->password = strdup(value);

		if (strcmp(key, "level") == 0) {
			level = atoi(value);
			if (level != 0) {
				user->level = level;
			}
		}

		int poem_len;
		int readed;
		char *poem;
		if (strcmp(key, "poem") == 0) {
			poem_len = atoi(value);
			poem = user->poem;

			do {
				readed = fread(poem, sizeof(char), MAX_POEM_LEN, file);
				poem += readed;
				poem_len -= readed;
			} while (readed < poem_len);
		}

	}
	fclose(file);
	return 0;
}

int dump_user_struct(User *user)
{
	if (!user) {
		return -1;
	}

	FILE *file = open_user_directory(user->login, "w");
	if (file == NULL) {
		return -1;
	}

	int ret;
	fprintf(file, "password %s\n", user->password);
	fprintf(file, "level %d\n", user->level);
	fprintf(file, "poem %lu\n", strlen(user->poem));
	if (strlen(user->poem) != 0) {
		ret = fwrite(user->poem, sizeof(char), strlen(user->poem), file);
		if (ret == 0) {
			perror("Error dumping user struct: ");
		}
	}

	fclose(file);
	return 0;
}

void init_pool()
{
	int size = POOL_START_SIZE*sizeof(User);
	user_pool = (User *)mmap((void *)0, size, PROT_READ|PROT_WRITE, MAP_PRIVATE|MAP_ANONYMOUS, -1, 0);
	users_allocated = 0;
	if (user_pool == MAP_FAILED) {
		perror("Failed to initialize pool");
		exit(1);
	}
}

User *user_allocator() {
	User *result;
	if (users_allocated < POOL_START_SIZE) {
		result = user_pool+users_allocated;
		users_allocated++;
	} else {
		printf("Not enough memory.\n Try to reconnect");
		fflush(stdout);
		result = NULL;
	}

	return result;
}

void free_pool() {
	int i;
	User *cur_user;
	for (i = 0; i < users_allocated; i++) {
		cur_user = &user_pool[i];
		if (cur_user->login != NULL)
			free(cur_user->login);
		if (cur_user->password != NULL)
			free(cur_user->password);
	}
}
