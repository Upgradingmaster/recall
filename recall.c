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
int contig_id = 1;

char* get_sqlite_db_path();
size_t parse_num(char* s);
void parse_opts(int argc, char** argv);


char* query_errmsg; // sqlite3_free
char* query_sql_template; // do not free
char* query_sql; // free

void exit_sequence() {
    if (query_errmsg) sqlite3_free(query_errmsg);
    if (query_sql) free(query_sql);
    exit(0);
}


// Caller should reset contig_id to 0 once the sql query is done to maintain order
int sql_callback_print(void * v, int ncol, char ** colVal, char ** colName) {
    printf("%d.", contig_id++);

    int min_chars = 8;
    for (int i = 1 ; i < ncol ; i++) {
       printf("\t%*s : %s\n", min_chars, colName[i], colVal[i] ? colVal[i] : "NULL");
    }
    printf("\n");
    return 0;
}

int sql_create_main_table_if_not_exists() {
    // create table if not exists TableName (col1 typ1, ..., colN typN)
    query_sql_template = NULL;
    query_sql = "CREATE TABLE IF NOT EXISTS "MAIN_TABLE_NAME" ("
                        "id INTEGER PRIMARY KEY, "
                        "name TEXT NOT NULL, "
                        "comment TEXT, "
                        "time INTEGER DEFAULT (strftime('%s', 'now')));";
    rc = sqlite3_exec(db, query_sql, NULL, NULL, &query_errmsg);

    if (rc != SQLITE_OK) { return -1; }
    return 0;
}

int recall_list(size_t n) {
    printf("Recall!\n");

    size_t size;
    if (n == 0) {
        query_sql_template = "SELECT * FROM "MAIN_TABLE_NAME";";
        size = strlen(query_sql_template) + 1;
        query_sql = malloc(size);
    } else {
         query_sql_template = "SELECT * FROM "MAIN_TABLE_NAME" ORDER BY id ASC LIMIT %d;";
        size = snprintf(NULL, 0, query_sql_template, n) + 1; // Check : how much would it write ?
    }
    query_sql = malloc(size);
    snprintf(query_sql, size, query_sql_template, n);

    rc = sqlite3_exec(db, query_sql, sql_callback_print, NULL, &query_errmsg);
    contig_id = 1;


    if (rc != SQLITE_OK) { return -1; }
    return 0;
}

int recall_add(char* name, char* comment) {
    printf("Add!\n");
                  
    query_sql_template = "INSERT INTO "MAIN_TABLE_NAME" (name, comment) VALUES ('%s', '%s');";
                  
    size_t size = snprintf(NULL, 0, query_sql_template, name, comment) + 1; // Check : how much would it write ?
    query_sql = malloc(size);
    snprintf(query_sql, size, query_sql_template, name, comment);
                  
    rc = sqlite3_exec(db, query_sql, NULL, NULL, &query_errmsg);

    if (rc != SQLITE_OK) { 
        return -1;
    }
    return 0;
}

int recall_remove(size_t idx) {
    printf("Remove!\n");

    if (idx == 0) idx = 1;

    query_sql_template = "DELETE FROM "MAIN_TABLE_NAME" WHERE id = (SELECT id FROM "MAIN_TABLE_NAME" ORDER BY id ASC LIMIT 1 OFFSET %d);";
;
    size_t size = snprintf(NULL, 0, query_sql_template, idx-1);
    query_sql   = malloc(size);
    snprintf(query_sql, size, query_sql_template, idx-1);

    rc = sqlite3_exec(db, query_sql, NULL , NULL, &query_errmsg);
    if (rc != SQLITE_OK){
        return -1;
    }
    return 0;
}
int recall_update(size_t idx, char* name, char* comment) {
    printf("Update!\n");
    if (idx == 0) idx = 1;

    query_sql_template = "UPDATE "MAIN_TABLE_NAME" SET name = '%s', comment = '%s'"
        "WHERE id = (SELECT id FROM "MAIN_TABLE_NAME" ORDER BY id ASC LIMIT 1 OFFSET %d);";
    size_t size = snprintf(NULL, 0, query_sql_template, name, comment, idx-1);
    query_sql   = malloc(size);
    snprintf(query_sql, size, query_sql_template, name, comment, idx-1);

    rc = sqlite3_exec(db, query_sql, NULL , NULL, &query_errmsg);
    if (rc != SQLITE_OK){
        return -1;
    }
    return 0;
}

int recall_clear() {
    printf("Clear!\n");

    query_sql_template = NULL;
    query_sql = "DELETE FROM "MAIN_TABLE_NAME"; DELETE FROM sqlite_sequence WHERE name = '"MAIN_TABLE_NAME"';";
    rc = sqlite3_exec(db, query_sql, NULL, NULL, &query_errmsg);

    if (rc != SQLITE_OK) {
        return -1;
    }
    return 0;
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
            if (strlen(opt) == 1) exit(-1); // Invalid Flag : Single "-"
                                     
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
                }
            } else if (strcmp(opt, "clear") == 0){
                ret = recall_clear();
            } else {
               printf("Invalid Keyword %s", opt);
            } 

            if (ret == -1) {
                printf("An action failed!\n");
                printf("Database Error: %s\n", query_errmsg);
                exit_sequence();
            }

            // We still continue, now only collecting keywords
        }
    }
}

// Returns 0 if s is NaN
size_t parse_num(char* s) {
    char* s_p;
    long num = strtol(s, &s_p, 10);
    if (*s_p == '\0') {
        return num;
    } else {
        return 0;
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
