
#define _GNU_SOURCE
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sqlite3.h>
#include <string.h>

#include <unistd.h>

#define DB_FILENAME "data.db"
#define APP_NAME "recall"
#define MAIN_TABLE_NAME "main"

#define SHIFT(argc, argv) (assert(argc), argc--, *argv++)

sqlite3 *db;
int rc;

char* get_sqlite_db_path();
void parse_opts(int argc, char** argv);



int sql_callback_print(void * v, int ncol, char ** colVal, char ** colName) {
    for (int i = 0 ; i < ncol ; i++) {
           printf("%s : %s\n", colName[i], colVal[i] ? colVal[i] : "NULL");
    }
    printf("\n");
    return 0;
}

void sql_create_main_table_if_not_exists() {
    // create table if not exists TableName (col1 typ1, ..., colN typN)
    char* errmsg;
    const char *sql = "CREATE TABLE IF NOT EXISTS "MAIN_TABLE_NAME" ("
                        "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                        "name TEXT NOT NULL, "
                        "comment TEXT, "
                        "time INTEGER DEFAULT (strftime('%s', 'now')));";

    rc = sqlite3_exec(db, sql, NULL, NULL, &errmsg);

    if (rc != SQLITE_OK) {
        printf("Sqlite: Error creating table: %s\n", errmsg);
        sqlite3_free(errmsg);
        sqlite3_close(db);
        exit(0);
    }
}

void recall_list() {
    printf("Recall!\n");
    char* errmsg;
    rc = sqlite3_exec(db, "select * from "MAIN_TABLE_NAME";", sql_callback_print, NULL, &errmsg);

    if (rc != SQLITE_OK) {
        printf("Sqlite Error: %s\n", errmsg);
        sqlite3_free(errmsg);
        sqlite3_close(db);
        exit(0);
    }
}

void recall_add(char* name, char* comment) {
    // TODO: Timestamp + SQL Injection + Switch away from GNU Extension
    printf("Add!\n");
    char* errmsg; // TODO: Factor this and others out
    char* sql;
    asprintf(&sql, "insert into "MAIN_TABLE_NAME" (name, comment) values ('%s', '%s');", name, comment);
    rc = sqlite3_exec(db, sql, sql_callback_print, NULL, &errmsg);

    if (rc != SQLITE_OK) { // TODO: Factor this and others out
        printf("Sqlite Error: %s\n", errmsg);
        sqlite3_free(errmsg);
        sqlite3_close(db);
        exit(0);
    }

}
void recall_remove() {
    printf("Remove!\n");

}
void recall_update() {
    printf("Update!\n");
}
void recall_clear() {
    printf("Clear\n");

    char* errmsg;
    char* sql = "delete from "MAIN_TABLE_NAME"; delete from sqlite_sequence where name = '"MAIN_TABLE_NAME"';";
    rc = sqlite3_exec(db, sql, sql_callback_print, NULL, &errmsg);

    if (rc != SQLITE_OK) {
        printf("Sqlite Error: %s\n", errmsg);
        sqlite3_free(errmsg);
        sqlite3_close(db);
        exit(0);
    }
}


int main(int argc, char** argv) {
    char* db_path = get_sqlite_db_path();
    if (sqlite3_open(db_path, &db)) {
        fprintf(stderr, "Could not open database connection: %s\n", sqlite3_errmsg(db));
        sqlite3_close(db);
        return -1;
    }

    sql_create_main_table_if_not_exists();
    parse_opts(argc, argv);


    sqlite3_close(db);
    free(db_path);
    return 0;


}

/* 
 * Consumes argv
 */
void parse_opts(int argc, char** argv) {
    char* name = SHIFT(argc, argv);
    int collectingFlags = 1;

    while (argc) {
        char* opt = SHIFT(argc, argv); 

        if(opt[0] == '-' && collectingFlags) { // Collect flag
            if (strlen(opt) == 1) exit(-1); // Invalid Flag : "-\0"
                                     
            switch(opt[1]) {
                case 'd': 
                    printf("Flag: Debug\n");
                    break;
                default: 
                    printf("Invalid Flag: %s\n", opt);
                    break;
            }
        } else { // Colect Keyword
            collectingFlags = 0; // Dont't accept flags any more 
            if (opt[0] == '-') {printf("Invalid Syntax\n"); exit(-1);} // flags come first
             
            if (strcmp(opt, "list") == 0) {
                recall_list();
            } else if (strcmp(opt, "add") == 0) {
                char* name = SHIFT(argc, argv); 
                char* comment = SHIFT(argc, argv); 
                recall_add(name, comment);
                
            } else if (strcmp(opt, "remove") == 0) {
                recall_remove();
            } else if (strcmp(opt, "update") == 0) {
                recall_update();
            } else if (strcmp(opt, "clear") == 0){
                recall_clear();
            } else {
               printf("Invalid Keyword %s", opt);
            }
            // We still continue, now only collecting keywords
        }
    }
}

char* get_sqlite_db_path() {
    char* db_path = NULL;
    char* data_dir = NULL;
    
    char* xdg_data_home = getenv("XDG_DATA_HOME");
    if (xdg_data_home != NULL && xdg_data_home[0] != '\0') { // XDG
        data_dir = xdg_data_home;
    } else { //  ~/.local/share/
        const char* home = getenv("HOME");
        if (home != NULL) {
            size_t len = strlen(home) + strlen("/.local/share") + 1;
            data_dir = malloc(len);
            if (data_dir != NULL) {
                snprintf(data_dir, len, "%s/.local/share", home);
            }
        }
    }
    
    if (data_dir == NULL) {
        return NULL;
    }
    
    // Create the app directory path
    size_t app_dir_len = strlen(data_dir) + strlen("/") + strlen(APP_NAME) + 1;
    char* app_dir = malloc(app_dir_len);
    if (app_dir == NULL) {
        free(data_dir);
        return NULL;
    }
    snprintf(app_dir, app_dir_len, "%s/%s", data_dir, APP_NAME);
    
    mkdir(data_dir, 0755);
    mkdir(app_dir, 0755);

    /* Build the final path */
    size_t db_path_len = app_dir_len + strlen("/") + strlen(DB_FILENAME) + strlen(".db") + 1;
    db_path = malloc(db_path_len);
    if (db_path == NULL) {
        free(app_dir);
        free(data_dir);
        return NULL;
    }
    snprintf(db_path, db_path_len, "%s/%s", app_dir, DB_FILENAME);
    free(app_dir);

    if (access(db_path, F_OK | R_OK | W_OK) != 0) {
        // file doesn't exist
        printf("Sqlite database doesn't exist, creating at %s\n", db_path);
        fclose(fopen(db_path, "w")); // Create it
    }

    return db_path;
}
