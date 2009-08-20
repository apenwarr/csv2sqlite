#include "wvcsv.h"
#include "wvfdstream.h"
#include <sqlite3.h>
#include <assert.h>

static int _exec(sqlite3 *db, WvError &err, WvStringParm fn, WvStringParm sql)
{
    char *errmsg = NULL;
    int rv = sqlite3_exec(db, sql, NULL, NULL, &errmsg);
    if (rv)
	err.set_both(rv, WvString("%s: %s", fn, errmsg));
    return rv;
}


static int _exec(sqlite3 *db, WvError &err, WvStringParm fn, WVSTRING_FORMAT_DECL)
{
    return _exec(db, err, fn, WvString(WVSTRING_FORMAT_CALL));
}


int main(int argc, char **argv)
{
    if (argc < 3)
    {
	fprintf(stderr, "Usage: cat file.csv | %s <sqlite.db> <tablename>\n",
		argv[0]);
	return 1;
    }
    const char *sqfile = argv[1], *tabname = argv[2];
    
    WvFdStream in(dup(0));
    WvError err, dontcare;
    
    WvString firstline = in.getline(-1);
    assert(!strncmp(firstline, "TABLE ", 6));
    
    sqlite3 *db;
    int rv = sqlite3_open(sqfile, &db);
    if (rv)
	err.set_both(rv, sqlite3_errmsg(db));
    
    WvDynBuf b;
    WvString headerline = wvcsv_readline(in, b);
    assert(!!headerline);
    WvStringList headers;
    wvcsv_splitline_slow(headers, headerline.edit(), headerline.len());
    
    _exec(db, err, "synchronous", "pragma synchronous = no");
    _exec(db, err, "drop", "drop table if exists [%s]", tabname);
    _exec(db, err, WvString("create table(%s)", tabname),
	  "create table [%s] ([%s])", tabname, headers.join("],["));
    _exec(db, err, "begin tran", "begin transaction");
    
    WvStringList qm;
    for (int i = headers.count(); i > 0; i--)
	qm.append("?");
    
    sqlite3_stmt *stmt;
    WvString q("insert into [%s] ([%s]) values (%s)",
	       tabname, headers.join("],["), qm.join(","));
    //printf("stmt: '%s'\n", q.cstr());
    rv = sqlite3_prepare_v2(db, q, q.len(), &stmt, NULL);
    if (rv)
	err.set_both(rv, sqlite3_errmsg(db));
    
    char *line;
    std::vector<char*> l;
    while (err.isok() && (line = wvcsv_readline(in, b)) != NULL)
    {
	wvcsv_splitline(l, line, strlen(line));
	for (int i = 0; i < (int)l.size(); i++)
	{
	    if (l[i])
		rv = sqlite3_bind_text(stmt, i+1, l[i], strlen(l[i]),
				       SQLITE_STATIC);
	    else
		rv = sqlite3_bind_null(stmt, i+1);
	    if (rv)
		err.set_both(rv, sqlite3_errmsg(db));
	}
	
	do {
	    rv = sqlite3_step(stmt);
	} while (!rv);
	if (rv != SQLITE_DONE)
	    err.set_both(rv, sqlite3_errmsg(db));
	
	rv = sqlite3_reset(stmt);
	if (rv)
	    err.set_both(rv, sqlite3_errmsg(db));
    }
    
    sqlite3_finalize(stmt);
    
    if (err.isok())
	_exec(db, err, "commit", "commit transaction");
    else
    {
	_exec(db, err, "rollback", "rollback transaction");
	fprintf(stderr, "SQL error: %s\n", err.errstr().cstr());
    }
    
    sqlite3_close(db);
    
    return err.isok() ? 0 : 2;
}
