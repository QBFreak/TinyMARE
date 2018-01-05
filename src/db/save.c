/* db/save.c */

#include "externs.h"

#define DB_VERSION 2	// TinyMare Compressed Ram v2

extern int db_flags;
static void db_write_object(FILE *f, dbref i);


// Writes a compressed DBREF# to the file. Number of bytes written is 1 if
// (num < 0), or dbref_len for all other valid numbers.
//
void putref(FILE *f, dbref num)
{
  if(num < 0) {
    putchr(f, 0xFF);
    return;
  }

  if(dbref_len >= 2) {
    if(dbref_len >= 3) {
      if(dbref_len == 4)
        putchr(f, num >> 24);
      putchr(f, num >> 16);
    }
    putchr(f, num >> 8);
  }
  putchr(f, num);
}

// Writes a string to the file with terminating null byte.
//
void putstring(FILE *f, char *s)
{
  if(s)
    fputs(s, f);
  putchr(f, 0);
}

// Writes a list of dbref#s to the file, starting with the count.
//
void putlist(FILE *f, dbref *list)
{
  int a=0, *ptr;

  /* Store number of items in list */
  for(ptr=list;ptr && *ptr != NOTHING;ptr++,a++);
  putref(f, a-1);

  for(ptr=list;ptr && *ptr != NOTHING;ptr++)
    putref(f, *ptr);
}

static void sql_write_object(dbref i)
{
  ALIST *attr;
  ATRDEF *k;

  sqlite3_stmt *res;
  int result, zone, parents, children;
  char query[1024];

  // Look out for those blasted null pointers...
  if(Typeof(i) == TYPE_ROOM || Typeof(i) == TYPE_ZONE) {
    if(db[i].zone == NULL) {
      zone = NOTHING;
    } else {
      zone = *db[i].zone;
    }
  } else {
    zone = NOTHING;
  }

  if(db[i].parents == NULL) {
      parents = NOTHING;
  } else {
      parents = *db[i].parents;
  }

  if(db[i].children == NULL) {
      children = NOTHING;
  } else {
      children = *db[i].children;
  }

  /* Save object header */
  snprintf(query, 1024, "INSERT INTO objects VALUES (%i, '%s', %i, %i, %i, %i, %i, %i, %i, %i, %i, %li, %li, %i, %i, %i)",
    i,
    db[i].name,
    db[i].location,
    db[i].contents,
    db[i].exits,
    (db[i].link == HOME)?db_top: (db[i].link == AMBIGUOUS)?(db_top+1):db[i].link,
    db[i].next,
    db[i].owner,
    db[i].flags,
    db[i].plane,
    db[i].pennies,
    db[i].create_time,
    db[i].mod_time,
    parents,
    children,
    zone
  );
  rc = sqlite3_prepare_v2(msdb, query, -1, &res, 0);
  if (rc != SQLITE_OK) {
    log_error("SQL statement failed during '%s' table population: %s", "objects", sqlite3_errmsg(msdb));
    return;
  }
  result = sqlite3_step(res);
  if (result != SQLITE_DONE)
    log_error("Unexpected result populating '%s' table: %i", "objects", result);
  sqlite3_finalize(res);

  /* Write out special player information */
  if(Typeof(i) == TYPE_PLAYER) {
    snprintf(query, 1024, "INSERT INTO players VALUES (%i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %i, %li, %li, '%s', '%s')",
      i,
      db[i].data->class-1,
      db[i].data->rows,
      db[i].data->cols,
      db[i].data->tz,
      db[i].data->tzdst,
      db[i].data->gender,
      db[i].data->passtype,
      db[i].data->term,
      db[i].data->steps,
      db[i].data->sessions,
      db[i].data->age,
      db[i].data->last,
      db[i].data->lastoff,
      db[i].data->pass,
      db[i].data->powers
    );
    rc = sqlite3_prepare_v2(msdb, query, -1, &res, 0);
    if (rc != SQLITE_OK) {
      log_error("SQL statement failed during '%s' table population: %s", "players", sqlite3_errmsg(msdb));
      return;
    }
    result = sqlite3_step(res);
    if (result != SQLITE_DONE)
      log_error("Unexpected result populating '%s' table: %i", "players", result);
    sqlite3_finalize(res);
  }

  /* Write the attribute list */
  for(attr=db[i].attrs;attr;attr=attr->next) {
    snprintf(query, 1024, "INSERT INTO attributes VALUES (%i, %i, %i, '%s')",
      i,
      attr->obj,
      attr->num,
      attr->text
    );
    rc = sqlite3_prepare_v2(msdb, query, -1, &res, 0);
    if (rc != SQLITE_OK) {
      log_error("SQL statement failed during '%s' table population: %s", "attributes", sqlite3_errmsg(msdb));
        return;
    }
    result = sqlite3_step(res);
    if (result != SQLITE_DONE)
      log_error("Unexpected result populating '%s' table: %i", "attributes", result);
    sqlite3_finalize(res);
  }

  /* Save attribute definitions */
  for(k=db[i].atrdefs;k;k=k->next) {
    snprintf(query, 1024, "INSERT INTO attribute_definitions VALUES (%i, %i, %i, '%s')",
      i,
      k->atr.num,
      k->atr.flags,
      k->atr.name
    );
    rc = sqlite3_prepare_v2(msdb, query, -1, &res, 0);
    if (rc != SQLITE_OK) {
      log_error("SQL statement failed during '%s' table population: %s", "attribute_definitions", sqlite3_errmsg(msdb));
      return;
    }
    result = sqlite3_step(res);
    if (result != SQLITE_DONE)
      log_error("Unexpected result populating '%s' table: %i", "attribute_definitions", result);
    sqlite3_finalize(res);
  }
}

dbref sql_write()
{
  // long pos, pos2;
  int i, result;
  sqlite3_stmt *res;
  char query[1024];


  /* Initial database modifier flags */
  db_flags=0;

  if((unsigned long)time(NULL) > 0xFFFFFFFF)
    db_flags |= DB_LONGINT;  /* Time must be stored using 64-bits */

  /* Create all the database tables */
  rc = sqlite3_prepare_v2(msdb, "DROP TABLE IF EXISTS database", -1, &res, 0);
  if (rc != SQLITE_OK) {
    log_error("SQL statement failed during 'database' table drop: %s", sqlite3_errmsg(msdb));
    return 0;
  }
  result = sqlite3_step(res);
  if (result != SQLITE_DONE)
    log_error("Unexpected result dropping '%s' table: %i", "database", result);
  sqlite3_finalize(res);

  rc = sqlite3_prepare_v2(msdb, "CREATE TABLE database ("
    "engine STRING,"
    "DB_VERSION INTEGER,"
    "db_top INTEGER,"
    "db_flags INTEGER,"
    "NUM_POWS INTEGER"
  ")", -1, &res, 0);
  if (rc != SQLITE_OK) {
    log_error("SQL statement failed during 'database' table creation: %s", sqlite3_errmsg(msdb));
    return 0;
  }
  result = sqlite3_step(res);
  if (result != SQLITE_DONE)
    log_error("Unexpected result creating '%s' table: %i", "database", result);
  sqlite3_finalize(res);

  rc = sqlite3_prepare_v2(msdb, "DROP TABLE IF EXISTS objects", -1, &res, 0);
  if (rc != SQLITE_OK) {
    log_error("SQL statement failed during '%s' table drop: %s", "objects", sqlite3_errmsg(msdb));
    return 0;
  }
  result = sqlite3_step(res);
  if (result != SQLITE_DONE)
    log_error("Unexpected result dropping '%s' table: %i", "objects", result);
  sqlite3_finalize(res);

  rc = sqlite3_prepare_v2(msdb, "CREATE TABLE objects ("
      "dbref INTEGER,"
      "name STRING,"
      "location INTEGER,"
      "contents INTEGER,"
      "exits INTEGER,"
      "link INTEGER,"
      "next INTEGER,"
      "owner INTEGER,"
      "flags INTEGER,"
      "plane INTEGER,"
      "pennies INTEGER,"
      "create_time INTEGER,"
      "mod_time INTEGER,"
      "parents INTEGER,"
      "children INTEGER,"
      "zone INTEGER"
  ")", -1, &res, 0);
  if (rc != SQLITE_OK) {
    log_error("SQL statement failed during '%s' table creation: %s", "objects", sqlite3_errmsg(msdb));
    return 0;
  }
  result = sqlite3_step(res);
  if (result != SQLITE_DONE)
    log_error("Unexpected result creating '%s' table: %i", "objects", result);
  sqlite3_finalize(res);

  rc = sqlite3_prepare_v2(msdb, "DROP TABLE IF EXISTS players", -1, &res, 0);
  if (rc != SQLITE_OK) {
    log_error("SQL statement failed during '%s' table drop: %s", "players", sqlite3_errmsg(msdb));
    return 0;
  }
  result = sqlite3_step(res);
  if (result != SQLITE_DONE)
    log_error("Unexpected result dropping '%s' table: %i", "players", result);
  sqlite3_finalize(res);

  rc = sqlite3_prepare_v2(msdb, "CREATE TABLE players ("
      "dbref INTEGER,"
      "class INTEGER,"
      "rows INTEGER,"
      "cols INTEGER,"
      "tz INTEGER,"
      "tzdst INTEGER,"
      "gender INTEGER,"
      "passtype INTEGER,"
      "term INTEGER,"
      "steps INTEGER,"
      "sessions INTEGER,"
      "age INTEGER,"
      "last INTEGER,"
      "lastoff INTEGER,"
      "pass STRING,"
      "powers STRING"
  ")", -1, &res, 0);
  if (rc != SQLITE_OK) {
    log_error("SQL statement failed during '%s' table creation: %s", "players", sqlite3_errmsg(msdb));
    return 0;
  }
  result = sqlite3_step(res);
  if (result != SQLITE_DONE)
    log_error("Unexpected result creating '%s' table: %i", "players", result);
  sqlite3_finalize(res);

  rc = sqlite3_prepare_v2(msdb, "DROP TABLE IF EXISTS attributes", -1, &res, 0);
  if (rc != SQLITE_OK) {
    log_error("SQL statement failed during '%s' table drop: %s", "attributes", sqlite3_errmsg(msdb));
    return 0;
  }
  result = sqlite3_step(res);
  if (result != SQLITE_DONE)
    log_error("Unexpected result dropping '%s' table: %i", "attributes", result);
  sqlite3_finalize(res);

  rc = sqlite3_prepare_v2(msdb, "CREATE TABLE attributes ("
      "dbref INTEGER,"
      "object INTEGER,"
      "number INTEGER,"
      "text STRING"
  ")", -1, &res, 0);
  if (rc != SQLITE_OK) {
    log_error("SQL statement failed during '%s' table creation: %s", "attributes", sqlite3_errmsg(msdb));
    return 0;
  }
  result = sqlite3_step(res);
  if (result != SQLITE_DONE)
    log_error("Unexpected result creating '%s' table: %i", "attributes", result);
  sqlite3_finalize(res);

  rc = sqlite3_prepare_v2(msdb, "DROP TABLE IF EXISTS attribute_definitions", -1, &res, 0);
  if (rc != SQLITE_OK) {
    log_error("SQL statement failed during '%s' table drop: %s", "attribute_definitions", sqlite3_errmsg(msdb));
    return 0;
  }
  result = sqlite3_step(res);
  if (result != SQLITE_DONE)
    log_error("Unexpected result dropping '%s' table: %i", "attribute_definitions", result);
  sqlite3_finalize(res);

  rc = sqlite3_prepare_v2(msdb, "CREATE TABLE attribute_definitions ("
      "dbref INTEGER,"
      "number INTEGER,"
      "flags INTEGER,"
      "name STRING"
  ")", -1, &res, 0);
  if (rc != SQLITE_OK) {
    log_error("SQL statement failed during '%s' table creation: %s", "attribute_definitions", sqlite3_errmsg(msdb));
    return 0;
  }
  result = sqlite3_step(res);
  if (result != SQLITE_DONE)
    log_error("Unexpected result creating '%s' table: %i", "attribute_definitions", result);
  sqlite3_finalize(res);

  rc = sqlite3_prepare_v2(msdb, "DROP TABLE IF EXISTS config", -1, &res, 0);
  if (rc != SQLITE_OK) {
    log_error("SQL statement failed during '%s' table drop: %s", "config", sqlite3_errmsg(msdb));
    return 0;
  }
  result = sqlite3_step(res);
  if (result != SQLITE_DONE)
    log_error("Unexpected result dropping '%s' table: %i", "config", result);
  sqlite3_finalize(res);

  rc = sqlite3_prepare_v2(msdb, "CREATE TABLE config ("
      "number INTEGER,"
      "type INTEGER,"
      "var STRING"
  ")", -1, &res, 0);
  if (rc != SQLITE_OK) {
    log_error("SQL statement failed during '%s' table creation: %s", "config", sqlite3_errmsg(msdb));
    return 0;
  }
  result = sqlite3_step(res);
  if (result != SQLITE_DONE)
    log_error("Unexpected result creating '%s' table: %i", "config", result);
  sqlite3_finalize(res);

  rc = sqlite3_prepare_v2(msdb, "DROP TABLE IF EXISTS sitelocks", -1, &res, 0);
  if (rc != SQLITE_OK) {
    log_error("SQL statement failed during '%s' table drop: %s", "sitelocks", sqlite3_errmsg(msdb));
    return 0;
  }
  result = sqlite3_step(res);
  if (result != SQLITE_DONE)
    log_error("Unexpected result dropping '%s' table: %i", "sitelocks", result);
  sqlite3_finalize(res);

  rc = sqlite3_prepare_v2(msdb, "CREATE TABLE sitelocks ("
      "class INTEGER,"
      "host STRING"
  ")", -1, &res, 0);
  if (rc != SQLITE_OK) {
    log_error("SQL statement failed during '%s' table creation: %s", "sitelocks", sqlite3_errmsg(msdb));
    return 0;
  }
  result = sqlite3_step(res);
  if (result != SQLITE_DONE)
    log_error("Unexpected result creating '%s' table: %i", "sitelocks", result);
  sqlite3_finalize(res);

  /* Write database header */
  snprintf(query, 1024, "INSERT INTO database VALUES ('%s', %i, %i, %i, %i)",
    "MARE",
    DB_VERSION,
    db_top,
    db_flags,
    NUM_POWS
  );
  rc = sqlite3_prepare_v2(msdb, query, -1, &res, 0);
  if (rc != SQLITE_OK) {
    log_error("SQL statement failed during 'database' table population: %s", sqlite3_errmsg(msdb));
    return 0;
  }
  result = sqlite3_step(res);
  if (result != SQLITE_DONE)
    log_error("Unexpected result populating '%s' table: %i", "database", result);
  sqlite3_finalize(res);

  /* Select database reference number byte-length */
  dbref_len=(db_top+1 < 0xFF)?1:(db_top+1 < 0xFF00)?2:
             (db_top+1 < 0xFF0000)?3:4;

  /* Save configuration */
  for(i=0;db_rule[i].type;i++) {
    /* Config header */
    // putchr(f, db_rule[i].type);

    /* 4-byte length of configuration data will get filled in */
    // pos=ftell(f);
    // putnum(f, 0);
    // db_rule[i].save(f, db_rule[i].type);

    /* Check if any data has been written */
    // pos2=ftell(f);
    // if(pos2-pos < 5)
    //   fseek(f, -5, SEEK_CUR);  /* No configuration data, back up 5 bytes */
    // else {
    //   /* Fill in length */
    //   fseek(f, pos, SEEK_SET);
    //   putnum(f, pos2-pos-4);
    //   fseek(f, pos2, SEEK_SET);
    // }
  }
  // putchr(f, 0);

  /* Write database */
  for(i=0;i<db_top;i++) {
    if(Typeof(i) == TYPE_GARBAGE)
      continue;
    sql_write_object(i);
  }

  return db_top;
}

// Procedure to save the entire database to <filename>.
//
dbref db_write(FILE *f, char *filename)
{
  dbref dt = sql_write();
  long pos, pos2;
  int i;

  /* Initial database modifier flags */
  db_flags=0;

  if((unsigned long)time(NULL) > 0xFFFFFFFF)
    db_flags |= DB_LONGINT;  /* Time must be stored using 64-bits */

  /* Write database header */
  fprintf(f, "MARE");
  putchr(f, DB_VERSION);
  putnum(f, db_top);
  putnum(f, db_flags);
  putchr(f, NUM_POWS);
  /* Select database reference number byte-length */
  dbref_len=(db_top+1 < 0xFF)?1:(db_top+1 < 0xFF00)?2:
             (db_top+1 < 0xFF0000)?3:4;

  /* Save configuration */
  for(i=0;db_rule[i].type;i++) {
    /* Config header */
    putchr(f, db_rule[i].type);

    /* 4-byte length of configuration data will get filled in */
    pos=ftell(f);
    putnum(f, 0);
    db_rule[i].save(f, db_rule[i].type);

    /* Check if any data has been written */
    pos2=ftell(f);
    if(pos2-pos < 5)
      fseek(f, -5, SEEK_CUR);  /* No configuration data, back up 5 bytes */
    else {
      /* Fill in length */
      fseek(f, pos, SEEK_SET);
      putnum(f, pos2-pos-4);
      fseek(f, pos2, SEEK_SET);
    }
  }
  putchr(f, 0);

  /* Write database */
  for(i=0;i<db_top;i++) {
    if(Typeof(i) == TYPE_GARBAGE)
      continue;
    db_write_object(f, i);
  }

  /* Write closing -1 as end of list marker. Check for out-of-disk errors. */
  if(putchr(f, 0xFF) == EOF) {
    perror(filename);
    return 0;
  }

  if(dt != db_top)
    log_error("Object count mismatch between SQLite and TinyMARE databases!");
  return db_top;
}

static void db_write_object(FILE *f, dbref i)
{
  ALIST *attr;
  ATRDEF *k;

  /* Save object header */
  putref(f, i);
  putstring(f, db[i].name);
  putref(f, db[i].location);
  putref(f, db[i].contents);
  putref(f, db[i].exits);
  putref(f, (db[i].link == HOME)?db_top:
            (db[i].link == AMBIGUOUS)?(db_top+1):db[i].link);
  putref(f, db[i].next);
  putref(f, db[i].owner);
  putnum(f, db[i].flags);
  putnum(f, db[i].plane);
  putnum(f, db[i].pennies);

  if(db_flags & DB_LONGINT) {
    putlong(f, db[i].create_time);
    putlong(f, db[i].mod_time);
  } else {
    putnum(f, db[i].create_time);
    putnum(f, db[i].mod_time);
  }

  /* Save object lists */
  putlist(f, db[i].parents);
  putlist(f, db[i].children);
  if(Typeof(i) == TYPE_ROOM || Typeof(i) == TYPE_ZONE)
    putlist(f, db[i].zone);

  /* Write out special player information */
  if(Typeof(i) == TYPE_PLAYER) {
    putchr(f, db[i].data->class-1);
    putchr(f, db[i].data->rows);
    putchr(f, db[i].data->cols);
    putchr(f, (db[i].data->tzdst << 6) | ((db[i].data->tz & 0x1F) << 1) |
              (db[i].data->passtype << 7) | db[i].data->gender);
    putshort(f, db[i].data->term);
    putnum(f, db[i].data->steps);
    putnum(f, db[i].data->sessions);
    putnum(f, db[i].data->age);

    if(db_flags & DB_LONGINT) {
      putlong(f, db[i].data->last);
      putlong(f, db[i].data->lastoff);
    } else {
      putnum(f, db[i].data->last);
      putnum(f, db[i].data->lastoff);
    }

    fwrite(db[i].data->pass, db[i].data->passtype?20:10, 1, f);
    fwrite(db[i].data->powers, (NUM_POWS+3)/4, 1, f);
  }

  /* Write the attribute list */
  for(attr=db[i].attrs;attr;attr=attr->next) {
    putchr(f, attr->num);
    putref(f, attr->obj);
    putstring(f, attr->text);
  }
  putchr(f, 0);

  /* Save attribute definitions */
  for(k=db[i].atrdefs;k;k=k->next) {
    putchr(f, k->atr.num);
    putshort(f, k->atr.flags);
    putstring(f, k->atr.name);
  }
  putchr(f, 0);
}
