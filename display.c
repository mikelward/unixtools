#include "display.h"
#include "list.h"

#include <assert.h>
#include <stdio.h>

/*
 * 2 space margin between columns
 *
 * XXX
 * I prefer using two so that -x and -xF have the same column layout,
 * this also makes the -F and -O options easier to handle
 * but this doesn't match BSD ls, and probably shouldn't be hard-coded,
 * so think about how to make this better
 * */
const int outermargin = 2;

/**
 * Print a list in rows across the page, e.g.
 * 0  1  2
 * 3  4  5
 *
 * Strings in stringlist should all move the cursor the same number of characters,
 * i.e. should all have the same field "width".
 * TODO Make the width/fieldwidth/screenwidth/displaywidth terminology better.
 *
 * screenwidth is the width of the screen in columns.
 */
void printacross(StringList *list, int stringwidth, int screenwidth)
{
    if (list == NULL) {
        fprintf(stderr, "printacross: list is NULL\n");
        return;
    }
    int len = length(list);
    if (len == 0) {
        return;
    }

    int colwidth = stringwidth + outermargin;
    int cols = screenwidth / colwidth;
    cols = (cols) ? cols : 1;
    int col = 0;
    for (int i = 0; i < len; i++) {
        char *str = getitem(list, i);
        printf("%s", str);
        col++;
        if (col == cols) {
            printf("\n");
            col = 0;
        }
        else {
            printspaces(outermargin);
        }
    }
    if (col != 0) {
        printf("\n");
    }
}

/**
 * Print a list in columns down the page, e.g.
 * 0  2  4
 * 1  3  5
 *
 * Strings in stringlist should all move the cursor the same number of characters,
 * i.e. should all have the same field "width".
 *
 * screenwidth is the width of the screen in columns.
 */
void printdown(StringList *list, int stringwidth, int screenwidth)
{
    if (list == NULL) {
        fprintf(stderr, "printdown: list is NULL\n");
        return;
    }
    int len = length(list);
    if (len <= 0) {
        return;
    }

    int colwidth = stringwidth + outermargin;

    int maxcols = screenwidth / colwidth;
    maxcols = (maxcols) ? maxcols : 1;
    int rows = ceildiv(len, maxcols);
    int cols = ceildiv(len, rows);

    int col = 0;
    for (int row = 0; row < rows; row++) {
        for (col = 0; col < cols; col++) {
            int idx = col*rows + row;
            if (idx >= len) {
                break;
            }
            char *elem = getitem(list, idx);
            printf("%s", elem);
            if (col == cols-1) {
                break;
            }
            else {
                printspaces(outermargin);
            }
        }
        printf("\n");
    }
}

/* vim: set ts=4 sw=4 tw=0 et:*/
