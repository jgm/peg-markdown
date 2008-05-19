/**********************************************************************

  markdown_output.c - functions for printing Elements parsed by 
                      markdown_peg.
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

/* TODO remove */
static extensions = 0;

static void print_html_string(char *str, bool obfuscate);
static void print_html_element_list(element *list, bool obfuscate);
static void print_html_element(element elt, bool obfuscate);
static void print_latex_string(char *str);
static void print_latex_element_list(element *list);
static void print_latex_element(element elt);
static void print_groff_string(char *str);
static void print_groff_mm_element_list(element *list);
static void print_groff_mm_element(element elt, int count);

/**********************************************************************

  Utility functions for printing

 ***********************************************************************/

static int padded = 2;      /* Number of newlines after last output.
                               Starts at 2 so no newlines are needed at start.
                               */

static element *endnotes;   /* List of endnotes to print after main content. */
static int notenumber = 0;  /* Number of footnote. */

/* pad - add newlines if needed */
static void pad(int num) {
    while (num-- > padded)
        printf("%s","\n");;
    padded = num;
}

/**********************************************************************

  Functions for printing Elements as HTML

 ***********************************************************************/

/* print_html_string - print string, escaping for HTML  
 * If obfuscate selected, convert characters to hex or decimal entities at random */
static void print_html_string(char *str, bool obfuscate) {
    while (*str != '\0') {
        switch (*str) {
        case '&':
            printf("%s","&amp;");;
            break;
        case '<':
            printf("%s","&lt;");;
            break;
        case '>':
            printf("%s","&gt;");;
            break;
        case '"':
            printf("%s","&quot;");;
            break;
        default:
            if (obfuscate) {
                if (rand() % 2 == 0)
                    printf("&#%d;", (int) *str);
                else
                    printf("&#x%x;", (unsigned int) *str);
            }
            else
                putchar(*str);
        }
    str++;
    }
}

/* print_html_element_list - print a list of elements as HTML */
static void print_html_element_list(element *list, bool obfuscate) {
    while (list != NULL) {
        print_html_element(*list, obfuscate);
        list = list->next;
    }
}

/* print_html_element - print an element as HTML */
static void print_html_element(element elt, bool obfuscate) {
    int lev;
    char *contents;
    switch (elt.key) {
    case SPACE:
        printf("%s", elt.contents.str);
        break;
    case LINEBREAK:
        printf("%s","<br/>");;
        break;
    case STR:
        print_html_string(elt.contents.str, obfuscate);
        break;
    case ELLIPSIS:
        printf("%s","&hellip;");;
        break;
    case EMDASH:
        printf("%s","&mdash;");;
        break;
    case ENDASH:
        printf("%s","&ndash;");;
        break;
    case APOSTROPHE:
        printf("%s","&rsquo;");;
        break;
    case SINGLEQUOTED:
        printf("%s","&lsquo;");;
        print_html_element_list(elt.children, obfuscate);
        printf("%s","&rsquo;");;
        break;
    case DOUBLEQUOTED:
        printf("%s","&ldquo;");;
        print_html_element_list(elt.children, obfuscate);
        printf("%s","&rdquo;");;
        break;
    case CODE:
        printf("%s","<code>");;
        print_html_string(elt.contents.str, obfuscate);
        printf("%s","</code>");;
        break;
    case HTML:
        printf("%s", elt.contents.str);
        break;
    case LINK:
        if (strstr(elt.contents.link.url, "mailto:") == elt.contents.link.url)
            obfuscate = true;  /* obfuscate mailto: links */
        printf("<a href=\"");
        print_html_string(elt.contents.link.url, obfuscate);
        printf("\"");
        if (strlen(elt.contents.link.title) > 0) {
            printf(" title=\"");
            print_html_string(elt.contents.link.title, obfuscate);
            printf("\"");
        }
        printf("%s",">");;     
        print_html_element_list(elt.contents.link.label, obfuscate);
        printf("%s","</a>");;
        break;
    case IMAGE:
        printf("<img src=\"");
        print_html_string(elt.contents.link.url, obfuscate);
        printf("\" alt=\"");
        print_html_element_list(elt.contents.link.label, obfuscate);
        printf("\"");
        if (strlen(elt.contents.link.title) > 0) {
            printf(" title=\"");
            print_html_string(elt.contents.link.title, obfuscate);
            printf("\"");
        }
        printf("%s"," />");;
        break;
    case EMPH:
        printf("%s","<em>");;
        print_html_element_list(elt.children, obfuscate);
        printf("%s","</em>");;
        break;
    case STRONG:
        printf("%s","<strong>");;
        print_html_element_list(elt.children, obfuscate);
        printf("%s","</strong>");;
        break;
    case LIST:
        print_html_element_list(elt.children, obfuscate);
        break;
    case RAW:
        /* \001 is used to indicate boundaries between nested lists when there
         * is no blank line.  We split the string by \001 and parse
         * each chunk separately. */
        contents = strtok(elt.contents.str, "\001");
        print_html_element(markdown(contents, extensions), obfuscate);
        while ((contents = strtok(NULL, "\001")))
            print_html_element(markdown(contents, extensions), obfuscate);
        break;
    case H1: case H2: case H3: case H4: case H5: case H6:
        lev = elt.key - H1 + 1;  /* assumes H1 ... H6 are in order */
        pad(2);
        printf("<h%1d>", lev);
        print_html_element_list(elt.children, obfuscate);
        printf("</h%1d>", lev);
        padded = 0;
        break;
    case PLAIN:
        pad(1);
        print_html_element_list(elt.children, obfuscate);
        padded = 0;
        break;
    case PARA:
        pad(2);
        printf("%s","<p>");;
        print_html_element_list(elt.children, obfuscate);
        printf("%s","</p>");;
        padded = 0;
        break;
    case HRULE:
        pad(2);
        printf("%s","<hr />");;
        padded = 0;
        break;
    case HTMLBLOCK:
        pad(2);
        printf("%s", elt.contents.str);
        padded = 0;
        break;
    case VERBATIM:
        pad(2);
        printf("%s", "<pre><code>");
        print_html_string(elt.contents.str, obfuscate);
        printf("%s", "</code></pre>");
        padded = 0;
        break;
    case BULLETLIST:
        pad(2);
        printf("%s", "<ul>");
        padded = 0;
        print_html_element_list(elt.children, obfuscate);
        pad(1);
        printf("%s", "</ul>");
        padded = 0;
        break;
    case ORDEREDLIST:
        pad(2);
        printf("%s", "<ol>");
        padded = 0;
        print_html_element_list(elt.children, obfuscate);
        pad(1);
        printf("%s","</ol>");;
        padded = 0;
        break;
    case LISTITEM:
        pad(1);
        printf("%s","<li>");;
        padded = 2;
        print_html_element_list(elt.children, obfuscate);
        printf("%s","</li>");;
        padded = 0;
        break;
    case BLOCKQUOTE:
        pad(2);
        printf("%s","<blockquote>\n");;
        padded = 2;
        print_html_element_list(elt.children, obfuscate);
        pad(1);
        printf("%s","</blockquote>");;
        padded = 0;
        break;
    case REFERENCE:
        /* Nonprinting */
        break;
    case NOTE:
        /* if contents.str == 0, then print note; else ignore, since this
         * is a note block that has been incorporated into the notes list */
        if (elt.contents.str == 0) {
            endnotes = cons(elt, endnotes);
            ++notenumber;
            printf("<a class=\"noteref\" id=\"fnref%d\" href=\"#fn%d\" title=\"Jump to note %d\">[%d]</a>", 
                notenumber, notenumber, notenumber, notenumber);
        }
        break;
    default: 
        fprintf(stderr, "print_html_element encountered unknown element key = %d\n", elt.key); 
        exit(EXIT_FAILURE);
    }
}

static void print_html_endnotes(void) {
    int counter = 0;
    if (endnotes == NULL) {
        return;
    }
    printf("<hr/>\n<ol id=\"notes\">");
    endnotes = reverse(endnotes);
    while (endnotes != NULL) {
        counter++;
        pad(1);
        printf("<li id=\"fn%d\">\n", counter);
        padded = 2;
        print_html_element_list(endnotes->children, false);
        printf(" <a href=\"#fnref%d\" title=\"Jump back to reference\">[back]</a>", counter);
        pad(1);
        printf("%s","</li>");
        endnotes = endnotes->next;
    }
    pad(1);
    printf("%s","</ol>");;
}

/**********************************************************************

  Functions for printing Elements as LaTeX

 ***********************************************************************/

/* print_latex_string - print string, escaping for LaTeX */
static void print_latex_string(char *str) {
    while (*str != '\0') {
        switch (*str) {
          case '{': case '}': case '$': case '%':
          case '&': case '_': case '#':
            printf("\\%c", *str);
            break;
        case '^':
            printf("%s","\\^{}");;
            break;
        case '\\':
            printf("%s","\\textbackslash{}");;
            break;
        case '~':
            printf("%s","\\ensuremath{\\sim}");;
            break;
        case '|':
            printf("%s","\\textbar{}");;
            break;
        case '<':
            printf("%s","\\textless{}");;
            break;
        case '>':
            printf("%s","\\textgreater{}");;
            break;
        default:
            putchar(*str);
        }
    str++;
    }
}

/* print_latex_element_list - print a list of elements as LaTeX */
static void print_latex_element_list(element *list) {
    while (list != NULL) {
        print_latex_element(*list);
        list = list->next;
    }
}

/* print_latex_element - print an element as LaTeX */
static void print_latex_element(element elt) {
    int lev;
    int i;
    char *contents;
    switch (elt.key) {
    case SPACE:
        printf("%s", elt.contents.str);
        break;
    case LINEBREAK:
        printf("%s","\\\\\n");;
        break;
    case STR:
        print_latex_string(elt.contents.str);
        break;
    case ELLIPSIS:
        printf("%s","\\ldots{}");;
        break;
    case EMDASH: 
        printf("%s","---");;
        break;
    case ENDASH: 
        printf("%s","--");;
        break;
    case APOSTROPHE:
        printf("%s","'");;
        break;
    case SINGLEQUOTED:
        printf("%s","`");;
        print_latex_element_list(elt.children);
        printf("%s","'");;
        break;
    case DOUBLEQUOTED:
        printf("%s","``");;
        print_latex_element_list(elt.children);
        printf("%s","''");;
        break;
    case CODE:
        printf("%s","\\texttt{");;
        print_latex_string(elt.contents.str);
        printf("%s","}");;
        break;
    case HTML:
        /* don't print HTML */
        break;
    case LINK:
        printf("\\href{%s}{", elt.contents.link.url);
        print_latex_element_list(elt.contents.link.label);
        printf("%s","}");;
        break;
    case IMAGE:
        printf("\\includegraphics{%s}", elt.contents.link.url);
        break;
    case EMPH:
        printf("%s","\\emph{");;
        print_latex_element_list(elt.children);
        printf("%s","}");;
        break;
    case STRONG:
        printf("%s","\\textbf{");;
        print_latex_element_list(elt.children);
        printf("%s","}");;
        break;
    case LIST:
        print_latex_element_list(elt.children);
        break;
    case RAW:
        /* \001 is used to indicate boundaries between nested lists when there
         * is no blank line.  We split the string by \001 and parse
         * each chunk separately. */
        contents = strtok(elt.contents.str, "\001");
        print_latex_element(markdown(contents, extensions));
        while ((contents = strtok(NULL, "\001")))
            print_latex_element(markdown(contents, extensions));
        break;
    case H1: case H2: case H3:
        pad(2);
        lev = elt.key - H1 + 1;  /* assumes H1 ... H6 are in order */
        printf("%s","\\");;
        for (i = elt.key; i > H1; i--)
            printf("%s","sub");;
        printf("%s","section{");;
        print_latex_element_list(elt.children);
        printf("%s","}");;
        padded = 0;
        break;
    case H4: case H5: case H6:
        pad(2);
        printf("%s","\\noindent\\textbf{");;
        print_latex_element_list(elt.children);
        printf("%s","}");;
        padded = 0;
        break;
    case PLAIN:
        pad(1);
        print_latex_element_list(elt.children);
        padded = 0;
        break;
    case PARA:
        pad(2);
        print_latex_element_list(elt.children);
        padded = 0;
        break;
    case HRULE:
        pad(2);
        printf("%s","\\begin{center}\\rule{3in}{0.4pt}\\end{center}\n");;
        padded = 0;
        break;
    case HTMLBLOCK:
        /* don't print HTML block */
        break;
    case VERBATIM:
        pad(1);
        printf("%s","\\begin{verbatim}\n");;
        print_latex_string(elt.contents.str);
        printf("%s","\n\\end{verbatim}");;
        padded = 0;
        break;
    case BULLETLIST:
        pad(1);
        printf("%s","\\begin{itemize}");;
        padded = 0;
        print_latex_element_list(elt.children);
        pad(1);
        printf("%s","\\end{itemize}");;
        padded = 0;
        break;
    case ORDEREDLIST:
        pad(1);
        printf("%s","\\begin{enumerate}");;
        padded = 0;
        print_latex_element_list(elt.children);
        pad(1);
        printf("%s","\\end{enumerate}");;
        padded = 0;
        break;
    case LISTITEM:
        pad(1);
        printf("%s","\\item ");;
        padded = 2;
        print_latex_element_list(elt.children);
        printf("%s","\n");;
        break;
    case BLOCKQUOTE:
        pad(1);
        printf("%s","\\begin{quote}");;
        padded = 0;
        print_latex_element(markdown(elt.contents.str, extensions));
        printf("%s","\\end{quote}");;
        padded = 0;
        break;
    case NOTE:
        /* if contents.str == 0, then print note; else ignore, since this
         * is a note block that has been incorporated into the notes list */
        if (elt.contents.str == 0) {
            printf("%s","\\footnote{");;
            padded = 2;
            print_latex_element_list(elt.children);
            printf("%s","}");;
            padded = 0; 
        }
        break;
    case REFERENCE:
        /* Nonprinting */
        break;
    default: 
        fprintf(stderr, "print_latex_element encountered unknown element key = %d\n", elt.key); 
        exit(EXIT_FAILURE);
    }
}

/**********************************************************************

  Functions for printing Elements as groff (mm macros)

 ***********************************************************************/

static bool in_list_item = false; /* True if we're parsing contents of a list item. */

/* print_groff_string - print string, escaping for groff */
static void print_groff_string(char *str) {
    while (*str != '\0') {
        switch (*str) {
        case '\\':
            printf("%s","\\e");;
            break;
        default:
            putchar(*str);
        }
    str++;
    }
}

/* print_groff_mm_element_list - print a list of elements as groff ms */
static void print_groff_mm_element_list(element *list) {
    int count = 1;
    while (list != NULL) {
        print_groff_mm_element(*list, count);
        list = list->next;
        count++;
    }
}

/* print_groff_mm_element - print an element as groff ms */
static void print_groff_mm_element(element elt, int count) {
    int lev;
    char *contents;
    switch (elt.key) {
    case SPACE:
        printf("%s", elt.contents.str);
        padded = 0;
        break;
    case LINEBREAK:
        pad(1);
        printf("%s",".br");;
        padded = 0;
        break;
    case STR:
        print_groff_string(elt.contents.str);
        padded = 0;
        break;
    case ELLIPSIS:
        printf("%s","...");;
        break;
    case EMDASH:
        printf("%s","\\[em]");;
        break;
    case ENDASH:
        printf("%s","\\[en]");;
        break;
    case APOSTROPHE:
        printf("%s","'");;
        break;
    case SINGLEQUOTED:
        printf("%s","`");;
        print_groff_mm_element_list(elt.children);
        printf("%s","'");;
        break;
    case DOUBLEQUOTED:
        printf("%s","\\[lq]");;
        print_groff_mm_element_list(elt.children);
        printf("%s","\\[rq]");;
        break;
    case CODE:
        printf("%s","\\fC");;
        print_groff_string(elt.contents.str);
        printf("%s","\\fR");;
        padded = 0;
        break;
    case HTML:
        /* don't print HTML */
        break;
    case LINK:
        print_groff_mm_element_list(elt.contents.link.label);
        printf(" (%s)", elt.contents.link.url);
        padded = 0;
        break;
    case IMAGE:
        printf("%s","[IMAGE: ");;
        print_groff_mm_element_list(elt.contents.link.label);
        printf("%s","]");;
        padded = 0;
        /* not supported */
        break;
    case EMPH:
        printf("%s","\\fI");;
        print_groff_mm_element_list(elt.children);
        printf("%s","\\fR");;
        padded = 0;
        break;
    case STRONG:
        printf("%s","\\fB");;
        print_groff_mm_element_list(elt.children);
        printf("%s","\\fR");;
        padded = 0;
        break;
    case LIST:
        print_groff_mm_element_list(elt.children);
        padded = 0;
        break;
    case RAW:
        /* \001 is used to indicate boundaries between nested lists when there
         * is no blank line.  We split the string by \001 and parse
         * each chunk separately. */
        contents = strtok(elt.contents.str, "\001");
        print_groff_mm_element(markdown(contents, extensions), count);
        while ((contents = strtok(NULL, "\001")))
            print_groff_mm_element(markdown(contents, extensions), count);
        break;
    case H1: case H2: case H3: case H4: case H5: case H6:
        lev = elt.key - H1 + 1;
        pad(1);
        printf(".H %d \"", lev);
        print_groff_mm_element_list(elt.children);
        printf("\"");
        padded = 0;
        break;
    case PLAIN:
        pad(1);
        print_groff_mm_element_list(elt.children);
        padded = 0;
        break;
    case PARA:
        pad(1);
        if (!in_list_item || count != 1)
            printf("%s",".P\n");;
        print_groff_mm_element_list(elt.children);
        padded = 0;
        break;
    case HRULE:
        pad(1);
        printf("%s","\\l'\\n(.lu*8u/10u'");;
        padded = 0;
        break;
    case HTMLBLOCK:
        /* don't print HTML block */
        break;
    case VERBATIM:
        pad(1);
        printf("%s",".VERBON 2\n");;
        print_groff_string(elt.contents.str);
        printf("%s",".VERBOFF");;
        padded = 0;
        break;
    case BULLETLIST:
        pad(1);
        printf("%s",".BL");;
        padded = 0;
        print_groff_mm_element_list(elt.children);
        pad(1);
        printf("%s",".LE 1");;
        padded = 0;
        break;
    case ORDEREDLIST:
        pad(1);
        printf("%s",".AL");;
        padded = 0;
        print_groff_mm_element_list(elt.children);
        pad(1);
        printf("%s",".LE 1");;
        padded = 0;
        break;
    case LISTITEM:
        pad(1);
        printf("%s",".LI\n");;
        in_list_item = true;
        padded = 2;
        print_groff_mm_element_list(elt.children);
        in_list_item = false;
        break;
    case BLOCKQUOTE:
        pad(1);
        printf("%s",".DS I\n");;
        padded = 2;
        print_groff_mm_element(markdown(elt.contents.str, extensions), 1);
        pad(1);
        printf("%s",".DE");;
        padded = 0;
        break;
    case NOTE:
        /* if contents.str == 0, then print note; else ignore, since this
         * is a note block that has been incorporated into the notes list */
        if (elt.contents.str == 0) {
            printf("%s","\\*F\n");;
            printf("%s",".FS\n");;
            padded = 2;
            print_groff_mm_element_list(elt.children);
            pad(1);
            printf("%s",".FE\n");;
            padded = 1; 
        }
        break;
    case REFERENCE:
        /* Nonprinting */
        break;
    default: 
        fprintf(stderr, "print_groff_mm_element encountered unknown element key = %d\n", elt.key); 
        exit(EXIT_FAILURE);
    }
}

/**********************************************************************

  Parameterized function for printing an Element.

 ***********************************************************************/

void print_element(element elt, int format) {
    switch (format) {
    case HTML_FORMAT:
        print_html_element(elt, false);
        if (endnotes != NULL) {
            pad(2);
            print_html_endnotes();
        }
        break;
    case LATEX_FORMAT:
        print_latex_element(elt);
        break;
    case GROFF_MM_FORMAT:
        print_groff_mm_element(elt, 1);
        break;
    default:
        fprintf(stderr, "print_element - unknown format = %d\n", format); 
        exit(EXIT_FAILURE);
    }
}
