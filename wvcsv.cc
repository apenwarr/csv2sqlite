#include "wvcsv.h"
#include "wvstream.h"
#include "wvstrutils.h"
#ifndef assert
#include <assert.h>
#endif


WvString wvcsv_quote(WvStringParm s)
{
    if (s.isnull())
	return ""; // unquoted blankness means "null"
    else if (s == "")
	return "\"\""; // quoted blankness means "empty string"
    else if (strcspn(s, ",\"\r\n") < s.len())
    {
	// if it has quotes or commas, it needs to be quoted properly
	WvString s2;
	s2.setsize(s.len()*2 + 2 + 1);
	char *optr = s2.edit();
	*optr++ = '\"';
	for (const char *iptr = s; *iptr; iptr++)
	{
	    if (*iptr == '\"')
		*optr++ = *iptr;
	    *optr++ = *iptr;
	}
	*optr++ = '\"';
	*optr++ = 0;
	return s2;
    }
    else
	return s; // the string doesn't need quoting: leave it as-is
}


WvString wvcsv_quote(WVSTRING_FORMAT_DEFN)
{
    return wvcsv_quote(WvString(WVSTRING_FORMAT_CALL));
}


char *wvcsv_readline(WvStream &stream, WvBuf &buf)
{
    bool need_read = false;
    do
    {
	if (need_read)
	{
	    stream.runonce();
	    stream.read(buf, 16384);
	}
	
	size_t len = buf.used();
	const unsigned char *sptr = buf.peek(0, len), *eptr = sptr + len;
	bool inquote = false;
	for (const unsigned char *cptr = sptr; cptr < eptr; cptr++)
	{
	    if (*cptr == '"')
		inquote = !inquote;
	    else if (!inquote && *cptr == '\n')
	    {
		// end of a line
		len = cptr - sptr + 1;
		unsigned char *s = (unsigned char *)buf.get(len);
		assert(s[len-1] == '\n');
		s[len-1] = 0;
		if (len > 1 && s[len-2] == '\r')
		    s[len-2] = 0;
		return (char *)s;
	    }
	}
	
	// finished loop without finding anything
	need_read = true;
    } while (stream.isok());
    
    // if we get here, the stream is dead, so return whatever we can
    if (buf.used())
    {
	buf.put('\0');
	return (char *)buf.get(buf.used());
    }
    else
	return NULL;
}


char *wvcsv_dequote(const char *in, char *out, size_t *length)
{
    if (!*in)
    {
	if (length) *length = 0;
	return NULL; // unquoted blankness is a null string
    }
    else if (!strcmp(in, "\"\""))
    {
	*out = 0;
	if (length) *length = 0;
	return out; // quoted blankness is an empty string
    }
    else
    {
	char *optr = out;
	const char *cptr = in;

	// Ignore one leading double quote.  If it starts with a double quote,
	// it must have been quoted on the encoding side, so the wrapping
	// quotes must be discarded.  The trailing quote will be automatically
	// dropped because it won't have a pair.
	if (in[0] == '"')
	    cptr++;

	for (; *cptr; cptr++)
	{
	    if (*cptr == '"')
	    {
		if (cptr[1] == '"')
		{
		    cptr++;
		    *optr++ = '"';
		}
	    }
	    else
		*optr++ = *cptr;
	}
	*optr = 0;
	if (length) *length = optr - out;
	return out;
    }
}


inline char *dequote_or_not(char *tofutz, bool dequote, size_t *length)
{
    return dequote ? wvcsv_dequote(tofutz, tofutz, length) : tofutz;
}

void wvcsv_splitline(std::vector<char*> &l, std::vector<size_t> &ll,
		     char *s, size_t slen,
		     bool dequote_values)
{
    bool inquote = false;
    int istart = 0;
    
    l.clear();
    ll.clear();
    
    for (size_t i = 0; i < slen; i++)
    {
	if (s[i] == '"')
	    inquote = !inquote;
	else if (!inquote && s[i] == ',')
	{
	    // end of a column
	    s[i] = 0;
	    size_t length = i - istart;
	    char *p = dequote_or_not(s+istart, dequote_values, &length);
	    l.push_back(p);
	    ll.push_back(length);
	    //assert(!p || strlen(p) == length);
	    istart = i + 1;
	}
    }
    
    size_t xlength = slen - istart;
    l.push_back(dequote_or_not(s+istart, dequote_values, &xlength));
    ll.push_back(xlength);
    assert(l.size() == ll.size());
}


void wvcsv_splitline_slow(WvStringList &l, char *s, size_t slen)
{
    std::vector<char*> v;
    std::vector<size_t> lv;
    wvcsv_splitline(v, lv, s, slen);
    
    for (std::vector<char*>::iterator i = v.begin(); i < v.end(); i++)
	l.append(*i);
}


WvCsvIter::WvCsvIter(WvStream &_stream, bool expect_TABLE, bool expect_headers,
		     bool dequote_values)
    : stream(_stream), dequote(dequote_values)
{
    if (expect_TABLE)
    {
	firstline = stream.getline(-1);
	if (!firstline || strncmp(firstline, "TABLE ", 6) != 0)
	    err.set("First line should start with TABLE");
    }
    if (!firstline) firstline = "";
    
    if (expect_headers && err.isok())
    {
	// headerline must be a class member since headers[] is a bunch of
	// pointers into it
	headerline = wvcsv_readline(stream, buf);
	if (headerline)
	{
	    std::vector<size_t> lv;
	    wvcsv_splitline(headers, lv, headerline.edit(), headerline.len());
	}
	else
	    err.set("CSV header line missing");
    }
}


bool WvCsvIter::next()
{
    // 'line' contents are stored in buf, and will be invalidated on the next
    // call to wvcsv_readline
    if (!err.isok()) return false;
    char *line = wvcsv_readline(stream, buf);
    if (!line || !line[0] || !strcmp(line, "\r")) return false;
    wvcsv_splitline(cols, lengths, line, strlen(line), dequote);
    return true;
}
