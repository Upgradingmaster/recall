#ifndef RECALL_DEFINES
#define RECALL_DEFINES

extern int debug;

#define APP_NAME "recall"
#define DB_FILE_NAME "data.db"
#define MAIN_TABLE_NAME "main"

#define SHIFT(argc, argv) (assert(argc), argc--, *argv++)

#define recall_log(S, ...) do { if (debug) printf (S, ##__VA_ARGS__); } while (0)

#endif // RECALL_DEFINES
