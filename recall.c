#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include "defines.h"
#include "db.h"

static void mk_data_dir_if_not_exist();
static void parse_opts(int argc, char** argv);
static size_t parse_num(char* s);

char* data_dir_path;
char* db_path;

int main(int argc, char** argv) {
    mk_data_dir_if_not_exist();

    db_init(db_path);
    parse_opts(argc, argv);
    db_close();

    free(db_path);
    free(data_dir_path);
    return 0;
}

static void mk_data_dir_if_not_exist() {
    const char *xdg_data_home = getenv("XDG_DATA_HOME");
    assert(xdg_data_home);

    // Create the app directory
    int n_dir = strlen(xdg_data_home) + strlen(APP_NAME) + 3;
    data_dir_path = malloc(n_dir);
    snprintf(data_dir_path, n_dir, "%s/%s/", xdg_data_home, APP_NAME);

    struct stat st = {0};
    if (stat(data_dir_path, &st) == -1) {
        if (mkdir(data_dir_path, 0700) == -1) {
            perror("Failed to create app directory");
            exit(1);
        }
    }

    int n = strlen(data_dir_path) + sizeof("data.db") + 1;
    db_path = malloc(n);
    snprintf(db_path, n, "%s%s", data_dir_path, "data.db");
    // Creation handled by sqlite3

}

static void parse_opts(int argc, char** argv) {
    char* name = SHIFT(argc, argv);
    int   collecting_flags = 1;

    while (argc) {
        char* opt = SHIFT(argc, argv); 

        if(opt[0] == '-' && collecting_flags) { // Collect flags first
            if (strlen(opt) == 1) exit(-1); // Invalid Flag : Single "-"
                                     
            // TODO: debug global variable
            switch(opt[1]) {
                case 'd': 
                    printf("Flag: Debug\n");
                    break;
                default: 
                    printf("Invalid Flag: %s\n", opt);
                    break;
            }
        } else { // Colect Keyword
            collecting_flags = 0; // Dont't accept flags any more 
            if (opt[0] == '-') {printf("Syntax Error\n"); exit(-1);} // flags come first

            // Actions
            int ret = 1;
            if (strcmp(opt, "list") == 0) {
                size_t n = 0;
                if (argc >= 1) {
                    n = parse_num(SHIFT(argc, argv));
                }
                ret = recall_list(n); 

            } else if (strcmp(opt, "add") == 0) {
                if (argc >= 2) {
                    char* name = SHIFT(argc, argv); 
                    char* comment = SHIFT(argc, argv); 
                    ret = recall_add(name, comment);
                } else {
                    printf("Please provide a name and comment\n");
                    ret = -1;
                }
            } else if (strcmp(opt, "remove") == 0) {
                size_t n = 0;
                if (argc >= 1) {
                    n = parse_num(SHIFT(argc, argv));
                }
                ret = recall_remove(n);
            } else if (strcmp(opt, "update") == 0) {
                if (argc >= 3) {
                    size_t idx = parse_num(SHIFT(argc, argv));
                    char* name = SHIFT(argc, argv); 
                    char* comment = SHIFT(argc, argv); 
                    ret = recall_update(idx, name, comment);
                } else {
                    printf("Please provide a name, comment and index\n");
                    ret = -1;
                }
            } else if (strcmp(opt, "clear") == 0) {
                ret = recall_clear();
            } else {
               printf("Invalid Keyword %s\n", opt);
               ret = -1;
            } 

            db_cleanup();

            if (ret == -1) {
                printf("An action failed!\n");
                break;
            }

            // continue;
        }
    }
}

// Returns 0 if s is NaN
static size_t parse_num(char* s) {
    char* s_p;
    long num = strtol(s, &s_p, 10);
    if (*s_p == '\0') {
        return num;
    } else {
        return 0;
    }
}

