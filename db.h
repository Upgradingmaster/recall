#ifndef RECALL_DB
#define RECALL_DB

#include <stddef.h>

int recall_clear();
int recall_update(size_t idx, char* name, char* comment);
int recall_remove(size_t idx);
int recall_add(char* name, char* comment);
int recall_list(size_t n);

int db_init(char* db_path);
void db_close();

#endif // RECALL_DB
