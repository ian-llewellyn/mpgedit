/*
 * Generate gtkrc file during Windows installation.
 *
 * Copyright (C) 2003 Adam Bernstein
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

#include <stdio.h>
#include <string.h>

char buf[1024];

int main(int argc, char *argv[])
{
    char *cp;
    int i;
    FILE *fp;

    if (argc == 1) {
        fprintf(stderr, "usage: %s directory\n", argv[0]);
        return 1;
    }
    for (i=1; i<argc; i++) {
        strcat(buf, argv[i]);
    }

    fp = fopen("gtkrc", "w");
    if (!fp) {
        fprintf(stderr, "failed opening gtkrc output file\n");
        return 1;
    }

    fprintf(fp, "pixmap_path \"");
    for (cp=buf; *cp; cp++) {
        if (*cp == '\\') {
            fputc('/', fp);
        }
        else {
            fputc(*cp, fp);
        }
    }
    fprintf(fp, "\"\n");
    return 0;
}
