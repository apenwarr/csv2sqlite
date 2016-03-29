#ifndef __WVCSV_H
#define __WVCSV_H

#include "wvstring.h"
#include "wvbuf.h"
#include "wverror.h"
#include <vector>

class WvStream;
class WvStringList;

/**
 * Quote the given string using CSV-compatible parsing rules.
 * 
 * You can combine a series of such quoted strings into a valid "line" of CSV
 * by simply concatenating them with commas in between.
 * 
 * We only add actual quotation marks if leaving them out would make parsing
 * ambiguous.
 * 
 * It *is* valid for s to contain newline characters.  This will produce an
 * output CSV line that contains newlines, but parsing is not ambiguous because
 * the newlines will be inside quotes.
 * 
 * Note: for NULL pointers we produce an empty string ("").  For empty strings,
 * we produce the empty quoted string ("\"\"").  This arbitrary rule can be
 * used to differentiate between the two cases upon decoding, but a decoder
 * that doesn't know this rule can simply treat them both as empty strings.
 */
WvString wvcsv_quote(WvStringParm s);

// convenience function
WvString wvcsv_quote(WVSTRING_FORMAT_DECL);

/**
 * Reverse the quoting operation from wvcsv_quote().
 * 
 * Note: it is explicitly okay for 'out' to point at the same buffer as 'in',
 * since dequoting a string never increases its length.
 * 
 * Returns 'out', if the dequoted string is non-null, otherwise NULL.
 * 
 * WARNING: you probably shouldn't use this function directly.  The naive
 * implementation of just reading lines of text, splitting on commas, and
 * dequoting the columns is broken for lots of reasons.
 * 
 * Use wvcsv_readline() and wvcsv_splitline() instead.
 */
char *wvcsv_dequote(const char *in, char *out, size_t *length = NULL);

/**
 * Given one "line" of a CSV file, split and decode the columns into l.
 * 
 * WARNING: this function modifies 's' in order to go faster.  If you don't like
 * this, make a copy first.
 * 
 * WARNING: a "line" of a CSV file may contain newlines.  You should read the
 * file using wvcsv_readline() if you don't want to screw up.
 */
void wvcsv_splitline(std::vector<char*> &l, std::vector<size_t> &ll,
		     char *s, size_t slen,
		     bool dequote_values = true);

/**
 * Just like wvcsv_splitline, but filling a WvStringList.
 * 
 * This is slower than using a vector of char*, because it needs to make a copy
 * of all the strings.  But it's useful if you want to call things like join().
 */
void wvcsv_splitline_slow(WvStringList &l, char *s, size_t slen);

/**
 * Given a stream and a temporary buffer, return the next full CSV line.
 * 
 * This function reads the next valid CSV line from stream and returns that line.
 * The returned line is suitable for splitting with wvcsv_splitline().  Returns
 * NULL if the stream can't be read for any reason.  (If you want to know the
 * reason, check stream's error status.)
 * 
 * You need to pass the same value for 'buf' every time you call this function
 * on a given stream.  Upon return, buf may contain part of the next CSV line(s),
 * which are important in order to parse the next line.
 * 
 * WARNING: the returned buffer is a pointer directly into 'buf'.  Doing
 * any further operations on 'buf' (including get()) will make the returned
 * pointer invalid.  If this bothers you, copy it immediately into a WvString.
 * 
 * WARNING: after calling wvcsv_readline() on an input stream, the stream is
 * not really useful for reading anything else, because of the way we store
 * extra bytes in buf.
 */
char *wvcsv_readline(WvStream &stream, WvBuf &buf);


class WvCsvIter
{
    WvStream &stream;
    WvDynBuf buf;
    WvString headerline;
    bool dequote;
    mutable std::vector<char*> cols;
public:
    mutable std::vector<size_t> lengths;
    WvString firstline;
    WvError err;
    std::vector<char*> headers;
    
    WvCsvIter(WvStream &_stream, bool expect_TABLE, bool expect_headers,
	      bool dequote_values = true);
    bool next();
    
    std::vector<char*> *ptr() const
        { return &cols; }
    WvIterStuff(std::vector<char*>);
};


#endif // __WVCSV_H
