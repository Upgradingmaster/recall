#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include "defines.h"
#include "db.h"

static void build_paths();
static void mk_files_if_not_exist();

static void parse_cmd(int argc, char** argv);

char* app_path;
char* db_path;

int debug = 0;

int main(int argc, char** argv) {
    build_paths();
    mk_files_if_not_exist();

    db_init(db_path);
    parse_cmd(argc, argv);
    db_close();

    free(db_path);
    free(app_path);
    return 0;
}

static void build_paths() {

    const char *xdg_data_home = getenv("XDG_DATA_HOME");
    if (!xdg_data_home) {
        printf("XDG_DATA_HOME environment variable not set.\n");
        exit(1);
    }

    int app_n = strlen(xdg_data_home) + strlen(APP_NAME) + 3;
    app_path = malloc(app_n);
    snprintf(app_path, app_n, "%s/%s/", xdg_data_home, APP_NAME);

    int db_n = strlen(app_path) + sizeof(DB_FILE_NAME) + 1;
    db_path = malloc(db_n);
    snprintf(db_path, db_n, "%s%s", app_path, DB_FILE_NAME);

}
static void mk_files_if_not_exist() {
    struct stat st = {0};
    if (stat(app_path, &st) == -1) {
        if (mkdir(app_path, 0700) == -1) {
            perror("Failed to create app directory");
            exit(1);
        }
    }

    // creation of db is handled by sqlite
}

static void parse_cmd(int argc, char** argv) {
    char* name = SHIFT(argc, argv);
    int   collecting_flags = 1;

    while (argc) {
        char* opt = SHIFT(argc, argv); 

        if(opt[0] == '-' && collecting_flags) { // Collect flags first
            switch(opt[1]) {
                case 'd': 
                    recall_log("[INFO] Debug Mode Enabled.\n");
                    debug = 1;
                    break;
                default: 
                    recall_log("[ERROR] Invalid Flag: `%s`. Ignoring... \n", opt);
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
                    n = atoi(SHIFT(argc, argv));
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
                    n = atoi(SHIFT(argc, argv));
                }
                ret = recall_remove(n);
            } else if (strcmp(opt, "update") == 0) {
                if (argc >= 3) {
                    size_t idx = atoi(SHIFT(argc, argv));
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

            if (ret == 0) {
                printf("Done.\n");
            } else {
                printf("An action failed!\n");
                break;
            }

            // continue;
        }
    }
}

