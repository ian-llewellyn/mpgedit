/*
 * File names string to list of names conversion routine
 *
 * Copyright (C) 2004 Adam Bernstein. All Rights Reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA  02111-1307  USA.
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include "parsename.h"
/*
 * String of file names parser.
 * Rules: When a , appears in the string, white space is NOT a delimiter.
 *        \, is a literal comma
 *        When a , appears, spaces following the , are ignored.
 *        When no , appears, white space separates entries.
 * Examples:
 *        file1.mp3 file2.mp3   file3.mp3
 *           ->file1.mp3
 *           ->file2.mp3
 *           ->file3.mp3
 *        White Room.mp3, The Doors -- Love Her Gently.mp3, Your Woman.mp3,
 *          Name\,with\,Commas.mp3
 *           ->White Room.mp3
 *           ->The Doors -- Love Her Gently.mp3
 *           ->Your Woman.mp3
 *           ->Name,with,Commas.mp3
#define DEBUG 1
 */


/*
 * This function serves two purposes:
 *  When commas=0:  Just expand backslash character
 *  When commas>0:  Split snames into a list of strings at each
 *                  unprotected separator character.
 */
char **expand_stringnames_2list_sep(char *snames, char sep, int commas)
{
    char *cp;
    char sep2;
    char **list = NULL;
    int  i      = 0;
    int  j      = 0;
    int  quoted = 0;
    char *name  = NULL;
    int  err    = 0;
    int  maxstrlen;

    if (!snames || commas < 0) {
        return NULL;
    }
    maxstrlen = strlen(snames) + 1;
    sep2 = (sep == ' ') ? ',' : ' ';

    list = (char **) calloc(commas+2, sizeof(char *));
    if (!list) {
        err = 1;
        goto clean_exit;
    }

    name = (char *) calloc(1, maxstrlen);
    if (!name) {
        err = 1;
        goto clean_exit;
    }

    cp = snames;
    while (*cp) {
        if (*cp == '\\' && !quoted) {
            quoted = 1;
            cp++;
        }
        else {
            if (*cp && *cp == sep && !quoted && commas > 0) {
                name[j] = '\0';
                if (i > commas) {
                    err = 1;
                    goto clean_exit;
                }
                list[i++] = name;
                j = 0;
                name = (char *) calloc(1, maxstrlen);
                if (!name) {
                    err = 1;
                    goto clean_exit;
                }
                cp++;
                while (*cp && isspace(*cp)) {
                    cp++;
                }
            }
            else {
                if (quoted && *cp != sep && *cp != sep2) {
                    name[j++] = '\\';
                }
                name[j++] = *cp;
                cp++;
            }
            quoted = 0;
        }
    }
    list[i] = name;

clean_exit:
    if (err) {
        if (list) {
            for (i=0; list[i]; i++) {
                free(list[i]);
            }
        }
        free(list);
        list = NULL;
    }
    return list;
}


void expand_stringnames_2list_free(char **snames)
{
    int i;

    for (i=0; snames[i]; i++) {
        free(snames[i]);
    }
    free(snames);
}


char **expand_stringnames_2list(char *snames)
{
    int  has_comma = 0;
    int  quoted    = 0;
    int  spaces    = 0;
    char *cp;
    char **list;

    /*
     * Does the string have any unquoted commas?
     */
    for (cp = snames; *cp; cp++) {
        if (isspace(*cp)) {
            spaces++;
        }
        if (*cp == '\\' && !quoted) {
            quoted = 1;
        }
        else {
            if (*cp == ',' && !quoted) {
                has_comma++;
            }
            quoted = 0;
        }
    }
#ifdef DEBUG
    printf("has_comma: %d\n", has_comma);
#endif

    list = has_comma ? expand_stringnames_2list_sep(snames, ',', has_comma) :
                       expand_stringnames_2list_sep(snames, ' ', spaces);
    return list;
}


#ifdef DEBUG
int main(int argc, char *argv[])
{
    char **list;
    char *str;

    if (argc == 1) {
        printf("usage: %s text\n", argv[0]);
        return 1;
    }

    str = "Test\\1file.mp3";
    list = expand_stringnames_2list(str);
    printf("\nBefore: <%s>\nAfter : ", str);
    while (*list) {
        printf("<%s>\n", *list++);
    }
           

    str = "Zero\\  comma\\  count, \\so\\will\\NOT\\split string at , \\last\\token";
    list = expand_stringnames_2list_sep(str, '\0', 0);
    printf("\nBefore: <%s>\nAfter : ", str);
    while (*list) {
        printf("<%s>\n", *list++);
    }

    str = "NON ZERO\\  comma\\  count, \\so\\WILL\\split string at , \\last\\token";
    printf("\nBefore: <%s>\nAfter : ", str);
    list = expand_stringnames_2list_sep(str, ',', 2);
    while (*list) {
        printf("<%s>\n", *list++);
    }

    str = "this\\ is a  string\\, this is string\\ two";
    printf("\nBefore: <%s>\nAfter : ", str);
    list = expand_stringnames_2list(str);
    while (*list) {
        printf("<%s>\n", *list++);
    }

    list = expand_stringnames_2list(argv[1]);
    while (*list) {
        printf("<%s>\n", *list++);
    }
    return 0;
}
#endif
