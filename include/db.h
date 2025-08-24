#ifndef DB_H
#define DB_H

#include <sqlite3.h>

int db_init(const char *filename);
void db_close();
int db_increment(const char *iface, const char *ip);
int db_get_count(const char *iface, const char *ip, int *count);
int db_print_stats(const char *iface, int client_fd);

#endif
