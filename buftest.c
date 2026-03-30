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

int utf8_locale_available = 0;

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

int test_printtobuf_arabic(void)
{
    if (!utf8_locale_available) return 0;
    Buf *buf = newbuf();

    printtobuf("دليل", ESCAPE_NONE, buf);
    assert(bufscreenpos(buf) == 4);
    freebuf(buf);
    return 0;
}

int test_printtobuf_japanese(void)
{
    if (!utf8_locale_available) return 0;
    Buf *buf = newbuf();

    printtobuf("ディレクトリ", ESCAPE_NONE, buf);
    /* 6 double width characters == 12 characters */
    assert(bufscreenpos(buf) == 12);
    freebuf(buf);
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
    if (!utf8_locale_available) return 0;
    Buf *buf = newbuf();

    printwchartobuf(L'‽', ESCAPE_NONE, buf);
    /* don't care what bufpos is */
    /* ensure a single Unicode character has screen width 1 */
    assert(bufscreenpos(buf) == 1);
    freebuf(buf);
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
    freebuf(buf);
    return 0;
}

int test_printwchartobuf_newline_cescape(void)
{
    Buf *buf = newbuf();

    printwchartobuf(L'\n', ESCAPE_C, buf);
    assert(strcmp(bufstring(buf), "\\n") == 0);
    freebuf(buf);
    return 0;
}

int test_printwchartobuf_newline_question(void)
{
    Buf *buf = newbuf();

    printwchartobuf(L'\n', ESCAPE_QUESTION, buf);
    assert(strcmp(bufstring(buf), "?") == 0);
    freebuf(buf);
    return 0;
}

int test_printwchartobuf_control_noescape_width(void)
{
    /* Control chars via ESCAPE_NONE should have 0 display width */
    Buf *buf = newbuf();

    printwchartobuf(L'\n', ESCAPE_NONE, buf);
    assert(bufscreenpos(buf) == 0);
    freebuf(buf);
    return 0;
}

int test_printtobuf_invalid_utf8_cescape(void)
{
    if (!utf8_locale_available) return 0;
    /* Invalid UTF-8 byte 0xFF should be escaped as octal */
    Buf *buf = newbuf();
    char text[] = { 'a', (char)0xFF, 'b', '\0' };

    printtobuf(text, ESCAPE_C, buf);
    assert(strcmp(bufstring(buf), "a\\377b") == 0);
    assert(bufscreenpos(buf) == 6); /* a(1) + \377(4) + b(1) */
    freebuf(buf);
    return 0;
}

int test_printtobuf_invalid_utf8_question(void)
{
    if (!utf8_locale_available) return 0;
    /* Invalid UTF-8 byte should become ? */
    Buf *buf = newbuf();
    char text[] = { 'a', (char)0xFF, 'b', '\0' };

    printtobuf(text, ESCAPE_QUESTION, buf);
    assert(strcmp(bufstring(buf), "a?b") == 0);
    assert(bufscreenpos(buf) == 3);
    freebuf(buf);
    return 0;
}

int test_printtobuf_invalid_utf8_noescape(void)
{
    if (!utf8_locale_available) return 0;
    /* Invalid UTF-8 byte passed through raw */
    Buf *buf = newbuf();
    char text[] = { 'a', (char)0xFF, 'b', '\0' };

    printtobuf(text, ESCAPE_NONE, buf);
    assert(bufpos(buf) == 3);
    assert(bufscreenpos(buf) == 3);
    freebuf(buf);
    return 0;
}

int test_printtobuf_incomplete_utf8(void)
{
    if (!utf8_locale_available) return 0;
    /* Incomplete UTF-8: first byte of 2-byte sequence with no continuation */
    Buf *buf = newbuf();
    char text[] = { 'a', (char)0xC3, '\0' }; /* 0xC3 starts a 2-byte sequence */

    printtobuf(text, ESCAPE_C, buf);
    assert(strcmp(bufstring(buf), "a\\303") == 0);
    freebuf(buf);
    return 0;
}

int test_printtobuf_mixed_ascii_cjk(void)
{
    if (!utf8_locale_available) return 0;
    /* Mixed ASCII and CJK characters */
    Buf *buf = newbuf();

    printtobuf("aディb", ESCAPE_NONE, buf);
    /* a(1) + ディ(2 chars * 2 width) + b(1) = 6 display width */
    assert(bufscreenpos(buf) == 6);
    freebuf(buf);
    return 0;
}

int test_printtobuf_combining_character(void)
{
    if (!utf8_locale_available) return 0;
    /* Combining character (U+0301 combining acute accent) has 0 display width */
    Buf *buf = newbuf();

    printtobuf("e\xCC\x81", ESCAPE_NONE, buf); /* é as e + combining accent */
    /* e(1) + combining accent(0) = 1 display width */
    assert(bufscreenpos(buf) == 1);
    freebuf(buf);
    return 0;
}

/**
 * Try to set a UTF-8 locale.  Returns 1 on success, 0 on failure.
 */
int set_utf8_locale(void)
{
    if (setlocale(LC_ALL, "C.utf8")) return 1;
    if (setlocale(LC_ALL, "C.UTF-8")) return 1;
    if (setlocale(LC_ALL, "en_US.UTF-8")) return 1;
    return 0;
}

int main(int argc, char **argv)
{
    myname = "buftest";

    utf8_locale_available = set_utf8_locale();
    if (!utf8_locale_available) {
        fprintf(stderr, "warning: no UTF-8 locale available, skipping Unicode tests\n");
    }

    test_newbuf();
    test_append_string();
    test_printtobuf_arabic();
    test_printtobuf_japanese();
    test_printwchartobuf_ascii();
    test_printwchartobuf_unicode();
    test_printwchartobuf_backslash_noescape();
    test_printwchartobuf_backslash_cescape();
    test_printwchartobuf_backslash_question();
    test_printwchartobuf_newline_noescape();
    test_printwchartobuf_newline_cescape();
    test_printwchartobuf_newline_question();
    test_printwchartobuf_control_noescape_width();
    test_printtobuf_invalid_utf8_cescape();
    test_printtobuf_invalid_utf8_question();
    test_printtobuf_invalid_utf8_noescape();
    test_printtobuf_incomplete_utf8();
    test_printtobuf_mixed_ascii_cjk();
    test_printtobuf_combining_character();
    return 0;
}

