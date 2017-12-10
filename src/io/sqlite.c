/* io/sqlite.c */

/* 2017-12-10 - Code for sqlite3 database - QBFreak@qbfreak.net
 *  Sqlite database is a secondary database accessed via @sql and sql(), and
 *   does not store any of the MARE objects. It is merely a better place to
 *   store data than object attributes.
 */

/* Sqlite3 module, handles @sql command. */

#include "externs.h"

void do_sql(dbref player, char *sql) {
    if(!power(player, POW_SQL)) {
        notify(player, "#-1 Permission denied.");
        return;
    }

    if(!*sql) {
        notify(player, "You must specify a sql statement.");
        return;
    }

    sqlite3_stmt *res;

    rc = sqlite3_prepare_v2(sdb, sql, -1, &res, 0);

    if (rc != SQLITE_OK) {
        notify(player, "SQL statement failed: %s\n", sqlite3_errmsg(sdb));
        return;
    }

    int r = 0;  // Row buffer pointer
    int c = 0;  // Column text pointer
    int i = 0;  // Current column
    char row_out[8192];
    const char *col_text;

    while (sqlite3_step(res) == SQLITE_ROW) {
        // Zero the row buffer
        for (r=0;r<8192;r++)
            row_out[r] = 0;
        // Reset the column pointers
        c = 0;
        i = 0;
        // For loop to make sure we don't go over the end of our buffer
        for(r=0;r<8191;r++) {
            // Check and see if we're out of columns
            if (i == sqlite3_column_count(res)) {
                if (r > 0) {
                    // Remove the trailing space
                    row_out[r - 1] = 0;
                }
                // Break out of the for loop
                break;
            }
            // Check and see if we're starting a new column
            if (c == 0) {
                col_text = sqlite3_column_text(res, i);
            }
            // Check and see if we're at the end of a column
            if (col_text[c] == 0) {
                // Space delimiter
                row_out[r] = ' ';
                c = 0;
                i++;
            } else {
                // Grab the next character from the column data
                row_out[r] = col_text[c];
                c++;
            }
        }
        notify(player, "%s", row_out);
    }

    sqlite3_finalize(res);
}
