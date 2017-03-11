extern "C" {
    int sqlite_init();
    int create_users_database(const char*);
    int check_step_second(const char*, const char*);
    char *hellhash(const char*, const char*);
    int auth_step(const char*, const char*, char*);
}
