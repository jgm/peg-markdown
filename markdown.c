/**********************************************************************

  markdown.c - markdown in C using a PEG grammar. 
  (c) 2008 John MacFarlane (jgm at berkeley dot edu).

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

 ***********************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "markdown_peg.h"
#include "my_getopt-1.5/getopt.h"

extern char *strdup(const char *string);
static int extensions;

/**********************************************************************

  Main program and auxiliary functions.

  Reads input from files on command line, or from stdin
  if no arguments given.  Converts tabs to spaces using TABSTOP.
  Parses the result for references (References), and then again for
  conversion to HTML (Doc).  The parser actions print the converted
  HTML, so there is no separate printing step here.  Character encodings
  are ignored.

 ***********************************************************************/

#define VERSION "0.2.3"
#define COPYRIGHT "Copyright (c) 2008 John MacFarlane.\n" \
                  "License GPLv2+: GNU GPL version 2 or later <http://gnu.org/licenses/gpl.html>\n" \
                  "This is free software: you are free to change and redistribute it.\n" \
                  "There is NO WARRANTY, to the extent permitted by law."

#define TABSTOP 4
#define INCREMENT 4096  /* size of chunks in which to allocate memory */

/* print version and copyright information */
void version(char *progname)
{
  printf("%s version %s\n"
         "%s\n",
         progname,
         VERSION,
         COPYRIGHT);
}

/* print a help summary */
void help(char *progname)
{
  printf("Usage: %s [options] [FILE]...\n"
         "Options:\n"
         "-t FORMAT or --to FORMAT        convert to FORMAT (default is html)\n"
         "                                FORMAT = html|latex|groff-mm\n"
         "-o FILE or --output FILE        send output to FILE (default is stdout)\n"
         "-x[EXTS] or --extensions [EXTS] use syntax extensions (all if EXTS not specified)\n"
         "                                EXTS = smart, notes, ...\n"
         "-V or --version                 print program version and exit\n"
         "-h or --help                    show this message and exit\n",
         progname);
}

/* print usage information to stderr */
void usage(char *progname)
{
  fprintf(stderr,
          "Summary: %s [--help] [--version] [options] [FILE]...\n",
          progname);
}

int main(int argc, char * argv[]) {

    int numargs;            /* number of filename arguments */
    FILE *inputs[argc];     /* array of file pointers for inputs */
    int lastinp;            /* index of last file in inputs */
    int i;
    size_t buflength = 0;
    size_t maxlength = INCREMENT;

    char *inputbuf;

    FILE *input;
    char *curchar;
    int charstotab;
    char *progname = argv[0];
    int opt;
    /* the output filename is initially 0 (a.k.a. stdout) */
    char *outfilename = 0;
    char *format = 0;
    char *exts = 0;

    int output_format = HTML_FORMAT;
    element parsed_input; 

    char *shortopts = "Vhx::o:t:";
    /* long options list */
    struct option longopts[] =
    {
      /* name,        has_arg,           flag, val */ /* longind */
      { "version",    no_argument,       0,    'V' }, /*       0 */
      { "help",       no_argument,       0,    'h' }, /*       1 */
      { "output",     required_argument, 0,    'o' }, /*       2 */
      { "to",         required_argument, 0,    't' }, /*       3 */
      { "extensions", optional_argument, 0,    'x' }, /*       3 */
      /* end-of-list marker */
      { 0, 0, 0, 0 }
    };
    /* long option list index */
    int longind = 0;
    
    extensions = 0;

    /* parse all options from the command line */
    while ((opt = getopt_long_only(argc, argv, shortopts, longopts, &longind)) != -1)
        switch (opt) {
        case 'V': /* -version */
            version(progname);
            return 0;
        case 'h': /* -help */
            help(progname);
            return 0;
        case 'x': /* -extended */
            exts = optarg;
            if (exts == NULL) {
                extensions = 0xFFFFFF;  /* turn on all extensions */
                break;
            }
            exts = strtok(optarg, ",");
            while (exts != NULL) {
                if (strcmp(exts, "smart") == 0)
                    extensions = extensions | EXT_SMART;
                else if (strcmp(exts, "notes") == 0)
                    extensions = extensions | EXT_NOTES;
                else {
                    fprintf(stderr, "%s: Unknown extension '%s'\n", progname, exts);
                    exit(EXIT_FAILURE);
                }   
                exts = strtok(NULL, ",");
            }
            break;
        case 't': /* -to */
            format = optarg;
            if (strcmp(format, "html") == 0)
                output_format = HTML_FORMAT;
            else if (strcmp(format, "latex") == 0)
                output_format = LATEX_FORMAT;
            else if (strcmp(format, "groff-mm") == 0)
                output_format = GROFF_MM_FORMAT;
            else {
                fprintf(stderr, "%s: Unknown output format '%s'\n", progname, format);
                exit(EXIT_FAILURE);
            }   
            break;
        case 'o': /* -output=FILE */
            outfilename = optarg;
            /* we allow "-" as a synonym for stdout here */
            if (strcmp(optarg, "-") == 0)
                outfilename = 0;
            break;
        default: /* something unexpected has happened */
            usage(progname);
            return 1;
        }

    /* re-open stdout to outfilename, if requested */
    if (outfilename)
        if (! freopen(outfilename, "w", stdout)) {
            perror(outfilename);
            return 1;
        }

    
    numargs = argc - optind; 
    if (numargs == 0) {        /* use stdin if no files specified */
       inputs[0] = stdin;
       lastinp = 0;
    }
    else {                  /* open all the files on command line */
       for (i = 0; i < numargs; i++)
            if ((inputs[i] = fopen(argv[optind + i], "r")) == NULL) {
                perror(argv[optind + i]);
                exit(EXIT_FAILURE);
            }
       lastinp = i - 1;
    }
    
    inputbuf = malloc(INCREMENT);
    curchar = inputbuf;
   
    for (i=0; i <= lastinp; i++) {
        input = inputs[i];
        charstotab = TABSTOP;
        while ((*curchar = fgetc(input)) != EOF) {
            switch (*curchar) {
            case '\t':
                while (charstotab > 0)
                   *curchar = ' ', curchar++, buflength++, charstotab--; 
                break;
            case '\n':
                curchar++, buflength++, charstotab = TABSTOP;
                break;
            default:
                curchar++, buflength++, charstotab--; 
            }
            if (charstotab == 0)
                charstotab = 4;
            if (buflength > maxlength - TABSTOP - 3) {
                maxlength += INCREMENT;
                inputbuf = realloc(inputbuf, maxlength);
                curchar = inputbuf + buflength;
                if (inputbuf == NULL) {
                    perror(progname);
                    exit(EXIT_FAILURE);
                }
            }
        }
        *curchar = 0;  /* terminate string (other EOF character) */
        fclose(input);
    }

    strcat(inputbuf, strdup("\n\n"));   /* add newlines to end to match Markdown.pl behavior */
   
    parsed_input = markdown(inputbuf, extensions);

    print_element(parsed_input, output_format, extensions);

    printf("\n");

    free(inputbuf);

    return(EXIT_SUCCESS);
}

