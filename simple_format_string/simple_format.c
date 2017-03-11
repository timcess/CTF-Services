#include <stdio.h>
#include <stdlib.h>

#define FLAG_LEN 1024
#define FLAG_COUNT 10
#define ID_LEN 12

char ids[FLAG_COUNT][ID_LEN];
char flags[FLAG_COUNT][FLAG_LEN];
size_t last_id = 0;
char *returning_flag = 0;

int generate_id() {
    //mutex lock
    last_id++;
    int lid = last_id;
    //mutex unlock

    int i;
    for (i = 0; i < ID_LEN; i++) {
        ids[lid][i] = (random() % 26) + 'a';
    }

    return lid;
}

void store_flag() {
    char buf[FLAG_LEN];
    int id;
    id = generate_id();

    printf("Your flag: ");
    scanf("%256s", flags[id]);
    printf("Flag successfuly stored!\n");
}

void get_flag() {
    char id[ID_LEN];
    int i;
    printf("Flag id: ");
    scanf("%12s", id);

    //mutex lock
    int lid = last_id;
    //mutext unlock

    for (i = 0; i < lid; i++) {
        if (strcmp(ids[i], id) == 0) {
            printf(flags[i]);
        }
    }
}

int main(int argc, char ** argv)
{
    char command;

    printf("Flag storage service is up again!\n");
    printf("1.Store flag\n");
    printf("2.Get falg\n");
    printf("3.Exit\n");

    while(1) {
        printf(">");
        scanf("%c", &command);
        switch(command) {
            case '1': store_flag(); break;
            case '2': get_flag(); break;
            case '3': return 0;
        }
    }

    return 0;
}
