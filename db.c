#include <sqlite3.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "defines.h"


sqlite3 *db;
int rc;
int contig_id = 1;

char* query_errmsg = NULL; // sqlite3_free
char* query_sql_template = NULL; // do not free
char* query_sql = NULL; // free


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

static int db_create_main_table_if_not_exists() {
    // create table if not exists TableName (col1 typ1, ..., colN typN)
    query_sql_template = NULL;
    query_sql = strdup("CREATE TABLE IF NOT EXISTS "MAIN_TABLE_NAME" ("
                        "id INTEGER PRIMARY KEY, "
                        "name TEXT NOT NULL, "
                        "comment TEXT, "
                        "time INTEGER DEFAULT (strftime('%s', 'now')));");
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

    rc = sqlite3_exec(db, query_sql, db_callback_print, NULL, &query_errmsg);
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
    query_sql = strdup("DELETE FROM "MAIN_TABLE_NAME";");
    rc = sqlite3_exec(db, query_sql, NULL, NULL, &query_errmsg);

    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", query_errmsg);
        return -1;
    }
    return 0;
}

int db_init(char* db_path) {
    if (sqlite3_open(db_path, &db) != SQLITE_OK) { printf("Could not open database connection: %s\n", sqlite3_errmsg(db)); sqlite3_close(db); return -1; }
    db_create_main_table_if_not_exists();
    return 0;
}

void db_cleanup() {
    if (query_errmsg) sqlite3_free(query_errmsg);
    if (query_sql) free(query_sql);
    query_errmsg = NULL;
    query_sql = NULL;
}

void db_close() {
    sqlite3_close(db);
}
