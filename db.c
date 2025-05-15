#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defines.h"


sqlite3 *db;
int rc;
int contig_id = 1;

char* format;
static int execute_sqlf(
        int (*callback)(void*,int,char**,char**), 
        void *data,
        const char* format, ...);

static int db_callback_print(void * v, int ncol, char ** colVal, char ** colName);

static int db_create_main_table_if_not_exists() {
    return execute_sqlf(NULL, NULL, "CREATE TABLE IF NOT EXISTS "MAIN_TABLE_NAME" ("
                        "id INTEGER PRIMARY KEY, "
                        "name TEXT NOT NULL, "
                        "comment TEXT, "
                        "time INTEGER DEFAULT (strftime('%s', 'now')));");
}

int recall_list(size_t n) {
    printf("Listing...\n");

    if (n == 0) {
        rc = execute_sqlf(db_callback_print, NULL,"SELECT * FROM "MAIN_TABLE_NAME";");
    } else {
        rc = execute_sqlf(db_callback_print, NULL, "SELECT * FROM "MAIN_TABLE_NAME" ORDER BY id ASC LIMIT %d;", n);
    }
    contig_id = 0;

    if (rc != SQLITE_OK) recall_log("[ERROR] sql error: %s\n", sqlite3_errmsg(db));
    return rc;
}

int recall_add(char* name, char* comment) {
    printf("Adding...\n");
    format = "INSERT INTO "MAIN_TABLE_NAME" (name, comment) VALUES ('%s', '%s');";
    rc = execute_sqlf(NULL, NULL, format, name, comment); 

    if (rc != SQLITE_OK) recall_log("[ERROR] sql error: %s\n", sqlite3_errmsg(db));
    return rc;
}

int recall_remove(size_t idx) {
    printf("Removing...\n");
    if (idx == 0) idx = 1;
    format = "DELETE FROM "MAIN_TABLE_NAME" WHERE id = (SELECT id FROM "MAIN_TABLE_NAME" ORDER BY id ASC LIMIT 1 OFFSET %d);";
    rc = execute_sqlf(NULL, NULL, format, idx-1);

    if (rc != SQLITE_OK) recall_log("[ERROR] sql error: %s\n", sqlite3_errmsg(db));
    return rc;
}

int recall_update(size_t idx, char* name, char* comment) {
    printf("Updating...\n");
    if (idx == 0) idx = 1;
    format = "UPDATE "MAIN_TABLE_NAME" SET name = '%s', comment = '%s'"
        "WHERE id = (SELECT id FROM "MAIN_TABLE_NAME" ORDER BY id ASC LIMIT 1 OFFSET %d);";
    rc = execute_sqlf(NULL, NULL, format, name, comment, idx-1);

    if (rc != SQLITE_OK) recall_log("[ERROR] sql error: %s\n", sqlite3_errmsg(db));
    return rc;
}

int recall_clear() {
    printf("Clearing...\n");
    rc = execute_sqlf(NULL, NULL, "DELETE FROM "MAIN_TABLE_NAME";");

    if (rc != SQLITE_OK) recall_log("[ERROR] sql error: %s\n", sqlite3_errmsg(db));
    return rc;
}

int db_init(char* db_path) {
    if (sqlite3_open(db_path, &db) != SQLITE_OK) { printf("Could not open database connection: %s\n", sqlite3_errmsg(db)); sqlite3_close(db); return -1; }
    db_create_main_table_if_not_exists();
    return 0;
}

void db_close() {
    sqlite3_close(db);
}

static int execute_sqlf(
        int (*callback)(void*,int,char**,char**), 
        void *data,
        const char* format, ...) {

    va_list args_1;
    va_start(args_1, format);

    va_list args_2;
    va_copy(args_2, args_1);

    size_t size = vsnprintf(NULL, 0, format, args_2) + 1; 
    va_end(args_2);

    char* query_sql = malloc(size);
    vsnprintf(query_sql, size, format, args_1);
    va_end(args_1);

    recall_log("[INFO] Running sql, `%s`\n", query_sql);
    rc = sqlite3_exec(db, query_sql, callback, data, NULL);

    free(query_sql);

    return rc;
}

// Caller should reset contig_id to 0 once the sql query is done to maintain order
static int db_callback_print(void * v, int ncol, char ** colVal, char ** colName) {
    printf("%d.", contig_id++);

    int min_chars = 8;
    for (int i = 1 ; i < ncol ; i++) {
       printf("\t%*s : %s\n", min_chars, colName[i], colVal[i] ? colVal[i] : "NULL");
    }
    printf("\n");
    return 0;
}
