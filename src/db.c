#include "db.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static sqlite3 *db = NULL;

int db_init(const char *filename) {
    if (sqlite3_open(filename, &db) != SQLITE_OK) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return -1;
    }

    const char *sql =
        "CREATE TABLE IF NOT EXISTS stats ("
        "iface TEXT NOT NULL,"
        "ip TEXT NOT NULL,"
        "count INTEGER NOT NULL,"
        "PRIMARY KEY (iface, ip));";

    char *errmsg = NULL;
    if (sqlite3_exec(db, sql, 0, 0, &errmsg) != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", errmsg);
        sqlite3_free(errmsg);
        return -1;
    }
    return 0;
}

void db_close() {
    if (db) sqlite3_close(db);
}

int db_increment(const char *iface, const char *ip) {
    const char *sql =
        "INSERT INTO stats (iface, ip, count) VALUES (?, ?, 1) "
        "ON CONFLICT(iface, ip) DO UPDATE SET count = count + 1;";

    sqlite3_stmt *stmt;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_text(stmt, 1, iface, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, ip, -1, SQLITE_STATIC);

    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
    return 0;
}

int db_get_count(const char *iface, const char *ip, int *count) {
    const char *sql = "SELECT count FROM stats WHERE iface=? AND ip=?;";
    sqlite3_stmt *stmt;
    *count = 0;

    if (sqlite3_prepare_v2(db, sql, -1, &stmt, 0) != SQLITE_OK) {
        return -1;
    }
    sqlite3_bind_text(stmt, 1, iface, -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, ip, -1, SQLITE_STATIC);

    if (sqlite3_step(stmt) == SQLITE_ROW) {
        *count = sqlite3_column_int(stmt, 0);
    }

    sqlite3_finalize(stmt);
    return 0;
}

int db_print_stats(const char *iface, int client_fd) {
    const char *sql_all = "SELECT iface, ip, count FROM stats;";
    const char *sql_iface = "SELECT iface, ip, count FROM stats WHERE iface=?;";

    sqlite3_stmt *stmt;
    if (iface) {
        if (sqlite3_prepare_v2(db, sql_iface, -1, &stmt, 0) != SQLITE_OK) return -1;
        sqlite3_bind_text(stmt, 1, iface, -1, SQLITE_STATIC);
    } else {
        if (sqlite3_prepare_v2(db, sql_all, -1, &stmt, 0) != SQLITE_OK) return -1;
    }

    char buf[256];
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        const unsigned char *iface_val = sqlite3_column_text(stmt, 0);
        const unsigned char *ip_val = sqlite3_column_text(stmt, 1);
        int cnt = sqlite3_column_int(stmt, 2);

        int n = snprintf(buf, sizeof(buf), "%s %s -> %d\n",
                         iface_val, ip_val, cnt);
        write(client_fd, buf, n);
    }

    sqlite3_finalize(stmt);
    return 0;
}
