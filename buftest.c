#define _XOPEN_SOURCE 600

#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <fcntl.h>
#include <locale.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "buf.h"
#include "logging.h"

int test_newbuf(void)
{
    Buf *buf = newbuf();

    /* newbuf sets shiftstate to initial shift state */
    assert(mbsinit(&buf->shiftstate));
    return 0;
}

int test_append_string(void)
{
    Buf *buf = newbuf();

    bufappend(buf, "abc", 3, 3);
    assert(strcmp(bufstring(buf), "abc") == 0);
    assert(bufpos(buf) == 3);
    assert(bufscreenpos(buf) == 3);
    return 0;
}

int test_printwchartobuf_ascii(void)
{
    Buf *buf = newbuf();

    printwchartobuf(L'a', ESCAPE_NONE, buf);
    assert(strcmp(bufstring(buf), "a") == 0);
    assert(bufpos(buf) == 1);
    assert(bufscreenpos(buf) == 1);
    return 0;
}

int test_printwchartobuf_unicode(void)
{
    Buf *buf = newbuf();

    printwchartobuf(L'‽', ESCAPE_NONE, buf);
    /* don't care what bufpos is */
    /* ensure a single Unicode character has screen width 1 */
    assert(bufscreenpos(buf) == 1);
    return 0;
}

int test_printwchartobuf_backslash_noescape(void)
{
    Buf *buf = newbuf();

    printwchartobuf(L'\\', ESCAPE_NONE, buf);
    assert(strcmp(bufstring(buf), "\\") == 0);
    return 0;
}

int test_printwchartobuf_backslash_cescape(void)
{
    Buf *buf = newbuf();

    printwchartobuf(L'\\', ESCAPE_C, buf);
    assert(strcmp(bufstring(buf), "\\\\") == 0);
    return 0;
}

int test_printwchartobuf_backslash_question(void)
{
    Buf *buf = newbuf();

    printwchartobuf(L'\\', ESCAPE_QUESTION, buf);
    assert(strcmp(bufstring(buf), "\\") == 0);
    return 0;
}

int test_printwchartobuf_newline_noescape(void)
{
    Buf *buf = newbuf();

    printwchartobuf(L'\n', ESCAPE_NONE, buf);
    assert(strcmp(bufstring(buf), "\n") == 0);
    return 0;
}

int test_printwchartobuf_newline_cescape(void)
{
    Buf *buf = newbuf();

    printwchartobuf(L'\n', ESCAPE_C, buf);
    assert(strcmp(bufstring(buf), "\\\n") == 0);
    return 0;
}

int test_printwchartobuf_newline_question(void)
{
    Buf *buf = newbuf();

    printwchartobuf(L'\n', ESCAPE_QUESTION, buf);
    assert(strcmp(bufstring(buf), "?") == 0);
    return 0;
}

int main(int argc, char **argv)
{
    myname = "buftest";

    setlocale(LC_ALL, "");

    test_newbuf();
    test_append_string();
    test_printwchartobuf_ascii();
    test_printwchartobuf_unicode();
    test_printwchartobuf_backslash_noescape();
    test_printwchartobuf_backslash_cescape();
    test_printwchartobuf_backslash_question();
    return 0;
}

