#include <cstdio>
#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <stdint.h>
#include <cstdlib>
#include <pthread.h>
#include <vector>
#include <cstring>
#include <string>
#include <sqlite3.h>
#include <unistd.h>
#include <signal.h>
#include <dirent.h>
#include <map>

#include "hellocator.h"
#include "help.h"


class Command;

class ClientHandler {
private:
    sqlite3 *_db;
    sqlite3 *_client_db;
    int _sfd;
    int _auth;
    uint64_t _free_space;
    uint64_t _occupied_space;
    std::string _login;
    void enter_logpass(std::string &login, std::string &pass);
    std::map <std::string, Command *> _commands;
    Command *_wrong_cmd;
public:
    ClientHandler(int client);
    ~ClientHandler();
    /* Communication */
    void send_msg(const char *msg);
    void send_msg(std::string msg);
    std::string recvline();
    std::string recvline_or_eof(int *);
    /* Commands */
    void space_info();
    void send_input_line();
    void send_welcome();
    void help();
    void register_client();
    void login();
    void allocate();
    void upload();
    void download();
    void list_files();
    void list_users();
    /* Others */
    Command *get_cmd(std::string &);
    bool check_credentials(std::string &login, std::string &pass, std::string &result);
};

class Command {
public:
    virtual int exec(ClientHandler &) = 0;
};

class Exit: public Command {
public:
    int exec(ClientHandler &ch) {
        ch.send_msg("Goodbye!");
        return 0;
    };
};

class Help: public Command {
public:
    int exec(ClientHandler &ch) {
        ch.help();
        return 1;
    };
};

class Login: public Command {
public:
    int exec(ClientHandler &ch) {
        ch.login();
        return 1;
    };
};

class Register: public Command {
public:
    int exec(ClientHandler &ch) {
        ch.register_client();
        return 1;
    };
};

class Allocate: public Command {
public:
    int exec(ClientHandler &ch) {
        ch.allocate();
        return 1;
    };
};

class Upload: public Command {
public:
    int exec(ClientHandler &ch) {
        ch.upload();
        return 1;
    };
};

class Download: public Command {
public:
    int exec(ClientHandler &ch) {
        ch.download();
        return 1;
    };
};

class ListFiles: public Command {
public:
    int exec(ClientHandler &ch) {
        ch.list_files();
        return 1;
    };
};

class SpaceInfo: public Command {
public:
    int exec(ClientHandler &ch) {
        ch.list_files();
        return 1;
    };
};

class ListUsers: public Command {
public:
    int exec(ClientHandler &ch) {
        ch.list_users();
        return 1;
    };
};

class Wrong: public Command {
public:
    int exec(ClientHandler &ch) {
        ch.send_msg("Wrong command\n");
        return 1;
    };
};


void
inline
ClientHandler:: send_input_line() {
    send_msg("->");
}

ClientHandler:: ClientHandler(int client) {
    _free_space = 0;
    _occupied_space = 0;
    _auth = 0;
    _sfd = client;
    _commands.insert({"HELP", new Help()});
    _commands.insert({"EXIT", new Exit()});
    _commands.insert({"REGISTER", new Register()});
    _commands.insert({"LOGIN", new Login()});
    _commands.insert({"LIST USERS", new ListUsers()});
    _commands.insert({"ALLOCATE", new Allocate()});
    _commands.insert({"UPLOAD", new Upload()});
    _commands.insert({"DOWNLOAD", new Download()});
    _commands.insert({"LIST FILES", new ListFiles()});
    _commands.insert({"SPACE INFO", new SpaceInfo()});
    _wrong_cmd = new Wrong();
    int dbdescr = sqlite3_open("db.db", &_db);
}

ClientHandler:: ~ClientHandler() {
    sqlite3_close(_db);
    sqlite3_close(_client_db);
}

Command *
ClientHandler::get_cmd(std::string &line) {
    auto it = _commands.find(line);
    if (it != _commands.end()) {
        return it->second;
    } else
        return _wrong_cmd;

}

static int callback_select(void *is_exist, int argc, char **argv, char **azColName) {
    if (argc > 1) *(bool *)is_exist = true;
    return 0;
}

static int callback_download(void *resvector, int argc, char **argv, char **azColName) {
    std::vector <std::string> *resv = reinterpret_cast<std::vector<std::string> *>(resvector);
    for (int i = 0; i < argc; i++) {
        resv->push_back(argv[i]);
    }
    return 0;
}

static int callback_list(void *resvector, int argc, char **argv, char **azColName) {
    std::vector <std::string> *resv = reinterpret_cast<std::vector<std::string> *>(resvector);
    for (int i = 0; i < argc; i++) {
        resv->push_back(argv[i]);
    }
    return 0;
}

static int callback_login(void *resvector, int argc, char **argv, char **azColName) {
    std::vector <std::string> *resv = reinterpret_cast<std::vector<std::string> *>(resvector);
    if (argc > 1) {
        resv->push_back(argv[0]);
        resv->push_back(argv[1]);
    }
    return 0;
}

static int callback_info(void *resvector, int argc, char **argv, char **azColName) {
    std::vector <uint64_t> *resv = reinterpret_cast<std::vector<uint64_t> *> (resvector);
    if (argc > 1) {
        resv -> push_back (atoll(argv[0]));
        resv -> push_back (atoll(argv[1]));
    }
    return 0;
}


void
ClientHandler:: download() {
    std::string resp, file;
    std::string login;
    std::string sql;
    sqlite3 *ddb;
    char *err;
    int dbres;
    std::vector <std::string> list_of_chunks;
    if (_auth == false) {
        send_msg("Log in firstly. No login - no flies\n");
        return;
    }
    send_msg("What file do you want download?\n");
    resp = recvline();
    ddb = _client_db;
    file = resp;

    sql = "SELECT * FROM '"+file+"';";
    dbres = sqlite3_exec(ddb, sql.c_str(), 0, 0, &err);
    if (dbres != SQLITE_OK)
    {
        sqlite3_free(err);
        send_msg("No such file\n");
        return;
    }
    sql = "SELECT CHUNK FROM '"+file+"' ORDER BY CHUNKID;";
    dbres = sqlite3_exec(ddb, sql.c_str(), callback_download, &list_of_chunks, &err);
    if (dbres != SQLITE_OK)
    {
        sqlite3_free(err);
        send_msg("No such file\n");
        return;
    }
    for (int i = 0; i < list_of_chunks.size(); i++)
        send_msg(list_of_chunks[i]);
    send_msg("\n");
}

void
ClientHandler:: allocate() {
    if (_auth == false)
    {
        send_msg("Log in firstly. No allocation for you.\n");
        return;
    }

    send_msg("Mkay, how much do you want to allocate?\n");
    uint32_t size_wanted = atoi(recvline().c_str());
    if (size_wanted + _free_space + _occupied_space > MAX_FREE_SPACE)
    {
        send_msg("You ask too much, sir.\n");
        return;
    }

    char *err;
    int dbres;
    std::string sql = "UPDATE spaceinfo " \
                      "SET FREESPACE=" + std::to_string(size_wanted + _free_space) + \
                      " WHERE FREESPACE=" + std::to_string(_free_space) + ";";
    dbres = sqlite3_exec(_client_db, sql.c_str(), 0, 0, &err);
    if (dbres != SQLITE_OK)
    {
        std::cout << "SQL error " << err << std::endl;
        sqlite3_free(err);
        send_msg("Allocation failed: technical problem.\n");
        return;
    }
    _free_space += size_wanted;
    send_msg("Here, take free memory!\n");
}


void
ClientHandler:: upload() {
    if (_auth == false)
    {
        send_msg("Log in firstly. No uploading for you.\n");
        return;
    }

    send_msg("Send us file's contents.\n");
    std::string file_content = recvline();
    uint32_t file_content_length = file_content.length();
    if (file_content_length > _free_space)
    {
        send_msg("ZOMG!! You tried to upload more than you can. Additional allocation required.\n");
        return;
    }

    send_msg("Which file do you want this to write in?\n");
    std::string file_name = recvline();
    if (file_name[0] >= '0' and file_name[0] <= '9')
    {
        send_msg("Filename cant start with number\n");
        return;
    }
    for (uint32_t i = 0; i < file_name.length(); ++i)
        if (not std::isalnum(file_name[i]))
        {
            send_msg("Something wrong in filename, check it for alphanumericalness.\n");
            return;
        }


    char *err;
    int dbres;
    std::string sql = "CREATE TABLE IF NOT EXISTS " + file_name + "("\
                      "CHUNKID INTEGER PRIMARY KEY AUTOINCREMENT,"\
                      "CHUNK CHAR(256),"\
                      "SIZE INTEGER);";
    dbres = sqlite3_exec(_client_db, sql.c_str(), 0, 0, &err);
    if (dbres != SQLITE_OK)
    {
        send_msg("Uploading failed: technical reason.\n");
        std::cerr << "SQL error: " << err << std::endl;
        sqlite3_free(err);
        return;
    }

    uint32_t pos = 0;
    for (uint32_t i = 0; i < file_content_length / 256; ++i)
    {
        sql = "INSERT INTO " + file_name + " (CHUNK, SIZE) "\
              "VALUES (" + file_content.substr(i, 256) + \
              ", 256);";
        dbres = sqlite3_exec(_client_db, sql.c_str(), 0, 0, &err);
        if (dbres != SQLITE_OK)
        {
            send_msg("Uploading failed: technical reason.\n");
            fprintf(stderr, "SQL error: %s\n", err);
            sqlite3_free(err);
            return;
        }
        _free_space -= 256;
        pos += 256;
    }

    uint32_t bytes_left = file_content_length % 256;
    if (bytes_left != 0)
    {
        sql = "INSERT INTO " + file_name + " (CHUNK, SIZE) "\
              "VALUES ('" + file_content.substr(pos) + \
              "', " + std::to_string(bytes_left) + ");";
        dbres = sqlite3_exec(_client_db, sql.c_str(), 0, 0, &err);
        if (dbres != SQLITE_OK)
        {
            send_msg("Uploading failed: technical reson.\n");
            std::cerr << "SQL error: " << err << std::endl;
            sqlite3_free(err);
            return;
        }
        sql = "INSERT INTO files (filename) VALUES ('"+file_name+"');";
        dbres = sqlite3_exec(_client_db, sql.c_str(), 0, 0, &err);
        if (dbres != SQLITE_OK)
        {
            send_msg("Uploading failed: technical reson.\n");
            std::cerr << "SQL error: " << err << std::endl;
            sqlite3_free(err);
            return;
        }
        _free_space -= bytes_left;
    }

    send_msg("Uploading went well, you may now continue.\n");
}


void
ClientHandler:: list_files()
{
    char *err;
    std::vector <std::string> list_of_names;
    if (_auth == false)
    {
        send_msg("Log in firstly. No login -> no space -> no files.\n");
        return;
    }

    std::string sql = "SELECT filename FROM files ORDER BY filename;";
    sqlite3_exec(_client_db, sql.c_str(), callback_list, &list_of_names, &err);

    for (int i = 0; i < list_of_names.size(); i++) {
        send_msg(list_of_names[i]+"\n");
    }

}

void
ClientHandler::list_users() {
    DIR *dfd;
    struct dirent *dp;

    dfd=opendir(USERS_DIR);

    while( (dp=readdir(dfd)) != NULL )
    {
        if (strcmp(dp->d_name, ".") && strcmp(dp->d_name, ".."))
        {
            send_msg(dp->d_name);
            send_msg("\n");
        }
    }

    closedir(dfd);
}

bool
ClientHandler::check_credentials(std::string &login, std::string &pass, std::string &result) {
    size_t i;
    if (!login.empty() && (i = login.find("\n")) != std::string::npos)
        login.erase(i);
    if (!pass.empty() && (i = pass.find("\n")) != std::string::npos)
        pass.erase(i);
    if (login.length() < 1) {
        result = "You should enter login";
        return false;
    }
    if (pass.length() < 1) {
        result = "You should enter password";
        return false;
    }
    if (pass.length() > PASSWORD_MAX_LEN) {
        result = "Password is too long O_o";
        return false;
    }
    if (login.length() > LOGIN_MAX_LEN) {
        result = "Login is too long";
        return false;
    }
    if (!check_step_second(login.c_str(), pass.c_str())) {
        result = "Login or password is not valid";
        return false;
    }
    return true;
}

void
ClientHandler:: help() {
    for (auto &item: _commands) {
        send_msg(item.first+"\n");
    }
}

void ClientHandler:: space_info() {
    if (_auth == false)
    {
        send_msg("Log in firstly. No login - No space.\n");
        return;
    }
    std::string str = "You have ";
    send_msg (str + std::to_string(MAX_FREE_SPACE - _free_space - _occupied_space) + " memory for allocation\n");
    send_msg (str + std::to_string(_free_space) + " memory to write files\n");
    send_msg (str + std::to_string(_occupied_space) + " memory already filled with files\n");
}


void
ClientHandler::enter_logpass(std::string &log, std::string &pass) {
    send_msg("Login: ");
    log = recvline();

    send_msg("Password: ");
    pass = recvline();
}

void
ClientHandler:: register_client() {
    std::string login, pass, res;
    enter_logpass(login, pass);

    if (check_credentials(login, pass, res)) {
        std::string sql = "SELECT * FROM user WHERE name='"+login+"';";
        char *err;
        int dbres;
        bool is_exist = false;
        dbres = sqlite3_exec(_db, sql.c_str(), callback_select, &is_exist, &err);
        if (dbres != SQLITE_OK) {
            std::cout << "SQL error " << err;
            sqlite3_free(err);
            res = "Registration failed: technical reason";
        }
        else
        {
            if (is_exist == true)
                res = "Login already exists";
            else
            {
                dbres = create_users_database(login.c_str());
                if (dbres)
                    res = "Registration failed: techical reason";
                else
                {
                    char *sumstr = hellhash(login.c_str(), pass.c_str());

                    std::string sum(sumstr);
                    std::string sql = "INSERT INTO user (name, password, sum) \
                                       VALUES ('"+login+"',"+"'"+pass+"',"+"'"+sum+"');";
                    dbres = sqlite3_exec(_db, sql.c_str(), 0, 0, &err);
                    if (dbres)
                    {
                        std::cout << "Registartion insert failed: " << err << std::endl;
                        sqlite3_free(err);
                        res = "Registration failed: technical reason";
                    }
                    else
                        res = "Success registration";

                    free(sumstr);
                }
            }
        }
    }
    send_msg(res+"\n");
}

void
ClientHandler:: login() {
    std::string login, pass, res;
    char *err;
    int dbres;
    std::vector <std::string> dbitems;

    _auth = true; //innocent, if not proven opposite

    enter_logpass(login, pass);
    if (not check_credentials(login, pass, res))
    {
        send_msg(res + "\n");
        _auth = false;
        return;
    }

    char *result = (char *)calloc(200, sizeof(char));
    if (auth_step(login.c_str(), pass.c_str(), result)) {
        send_msg(result);
        return;
    }

    std::string client_db_path = USERS_DIR + login;
    dbres = sqlite3_open(client_db_path.c_str(), &_client_db);
    if (dbres)
    {
        std::cerr << "Cannot open database " << client_db_path << std::endl;
        send_msg ("Login failed: technical reason.\n");
        _auth = false;
        return;
    }

    std::vector<uint64_t> db_info_items;
    std::string sql = "SELECT freespace, occupiedspace FROM spaceinfo;";
    dbres = sqlite3_exec (_client_db, sql.c_str(), callback_info, &db_info_items, &err);
    if (dbres) {
        std::cerr << "Cannot select from database " << client_db_path << ": " << err << std:: endl;
        send_msg ("Login failed: technical reason.\n");
        _auth = false;
        return;
    }
    std::cout << "User's freespace: " << _free_space << "\nUser's occupiedspace: " << _occupied_space << std::endl;

    if (db_info_items.size() != 2) {
        std::cout << "Something wrong went, db_info_items should have size 2" << std::endl;
        std::cout << db_info_items.size() << std::endl;
        for (int i = 0; i < db_info_items.size(); i++) {
            std::cout << db_info_items[i] << " ";
        }
        std::cout << std::endl;
        send_msg ("Login failed: technical reason.\n");
        _auth = false;
        return;
    }
    _free_space = db_info_items[0];
    _occupied_space = db_info_items[1];
    _auth = 1;
    _login = login;

    send_msg("Hello, "+login);
}


void
ClientHandler::send_welcome() {
    char art_welcome[WELCOME_LEN];
    memset(art_welcome, 0, WELCOME_LEN);
    std::ifstream stream_welcome(WELCOME_FILE);
    stream_welcome.read(art_welcome, WELCOME_LEN);
    send(_sfd, art_welcome, WELCOME_LEN, 0);
    stream_welcome.close();
}

void
inline
ClientHandler::send_msg(const char *msg) {
    send(_sfd, msg, strlen(msg), 0);
}

void
inline
ClientHandler::send_msg(std::string msg) {
    send(_sfd, msg.c_str(), msg.length(), 0);
}

std::string
ClientHandler::recvline() {
    std::string result = "";
    char chr;
    while (recv(_sfd, &chr, 1, 0) == 1) {
        if (chr == '\n')
            break;
        else
            result += chr;
    }
    return result;
}

std::string
ClientHandler::recvline_or_eof(int *eof) {
    std::string result = "";
    char chr;
    *eof = 0;

    while (recv(_sfd, &chr, 1, 0) == 1) {
        if (chr == '\n') {
            return result;
        }
        result += chr;
    }

    *eof = 1;
    return result;
}


void*
handler(void *clfd) {
    int sfd = *(int *)clfd;
    int eof;
    ClientHandler client(sfd);
    std::string response;
    Command *cmd;

    client.send_welcome();
    client.send_msg("Hello in Hellocator Center!\n");
    client.send_msg("Say, what do you want? May be you need HELP?\n");

    do {
        client.send_input_line();
        response = client.recvline_or_eof(&eof);
        if (eof) {
            std::string exit("EXIT");
            cmd = client.get_cmd(exit);
        } else {
            cmd = client.get_cmd(response);
        }
    } while (cmd->exec(client));

    close(sfd);
    pthread_exit((void *)0);
}

int main()
{
    int sfd, client;
    int retvalue, arg, yes;
    uint16_t port = 31337;
    struct sockaddr_in addr;
    struct sockaddr clients;
    struct sigaction act;
    socklen_t addr_len;
    pthread_t tidp;

    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = SIG_IGN;
    sigaction(SIGPIPE, &act, 0);

    sqlite_init();

    sfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sfd < 0) {
        perror("Socket create error: ");
        exit(1);
    }
    yes = 1;
    if (setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) != 0) {
        perror("Setsockopt error: ");
        exit(1);
    }
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    retvalue = bind(sfd, (struct sockaddr *)&addr, sizeof(addr));
    if (retvalue != 0) {
        perror("Bind error: ");
        exit(1);
    }
    retvalue = listen(sfd, 5);
    if (retvalue != 0) {
        perror("Listen error: ");
        exit(1);
    }

    addr_len = sizeof(clients);
    while (client=accept(sfd, (struct sockaddr *)&clients, &addr_len)) {
        if (client < 0) {
            perror("Accept error: ");
        }
        arg = client;
        pthread_create(&tidp, 0, handler, &arg);
        addr_len = sizeof(clients);
    }

    close(sfd);
    return 0;
}
