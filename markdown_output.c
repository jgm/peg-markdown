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

static extensions;

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

static FILE * stream = stdout;   /* Stream where output should be written */

/* pad - add newlines if needed */
static void pad(int num) {
    while (num-- > padded)
        fprintf(stream, "\n");;
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
            fprintf(stream, "&amp;");;;
            break;
        case '<':
            fprintf(stream, "&lt;");;;
            break;
        case '>':
            fprintf(stream, "&gt;");;;
            break;
        case '"':
            fprintf(stream, "&quot;");;;
            break;
        default:
            if (obfuscate) {
                if (rand() % 2 == 0)
                    fprintf(stream, "&#%d;", (int) *str);
                else
                    fprintf(stream, "&#x%x;", (unsigned int) *str);
            }
            else
                putc(*str, stream);
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
    element res;
    switch (elt.key) {
    case SPACE:
        fprintf(stream, "%s", elt.contents.str);
        break;
    case LINEBREAK:
        fprintf(stream, "<br/>");;;
        break;
    case STR:
        print_html_string(elt.contents.str, obfuscate);
        break;
    case ELLIPSIS:
        fprintf(stream, "&hellip;");;;
        break;
    case EMDASH:
        fprintf(stream, "&mdash;");;;
        break;
    case ENDASH:
        fprintf(stream, "&ndash;");;;
        break;
    case APOSTROPHE:
        fprintf(stream, "&rsquo;");;;
        break;
    case SINGLEQUOTED:
        fprintf(stream, "&lsquo;");;;
        print_html_element_list(elt.children, obfuscate);
        fprintf(stream, "&rsquo;");;;
        break;
    case DOUBLEQUOTED:
        fprintf(stream, "&ldquo;");;;
        print_html_element_list(elt.children, obfuscate);
        fprintf(stream, "&rdquo;");;;
        break;
    case CODE:
        fprintf(stream, "<code>");;;
        print_html_string(elt.contents.str, obfuscate);
        fprintf(stream, "</code>");;;
        break;
    case HTML:
        fprintf(stream, "%s", elt.contents.str);
        break;
    case LINK:
        if (strstr(elt.contents.link.url, "mailto:") == elt.contents.link.url)
            obfuscate = true;  /* obfuscate mailto: links */
        fprintf(stream, "<a href=\"");
        print_html_string(elt.contents.link.url, obfuscate);
        fprintf(stream, "\"");
        if (strlen(elt.contents.link.title) > 0) {
            fprintf(stream, " title=\"");
            print_html_string(elt.contents.link.title, obfuscate);
            fprintf(stream, "\"");
        }
        fprintf(stream, ">");;;
        print_html_element_list(elt.contents.link.label, obfuscate);
        fprintf(stream, "</a>");;;
        break;
    case IMAGE:
        fprintf(stream, "<img src=\"");
        print_html_string(elt.contents.link.url, obfuscate);
        fprintf(stream, "\" alt=\"");
        print_html_element_list(elt.contents.link.label, obfuscate);
        fprintf(stream, "\"");
        if (strlen(elt.contents.link.title) > 0) {
            fprintf(stream, " title=\"");
            print_html_string(elt.contents.link.title, obfuscate);
            fprintf(stream, "\"");
        }
        fprintf(stream, " />");;;
        break;
    case EMPH:
        fprintf(stream, "<em>");;;
        print_html_element_list(elt.children, obfuscate);
        fprintf(stream, "</em>");;;
        break;
    case STRONG:
        fprintf(stream, "<strong>");;;
        print_html_element_list(elt.children, obfuscate);
        fprintf(stream, "</strong>");;;
        break;
    case LIST:
        print_html_element_list(elt.children, obfuscate);
        break;
    case RAW:
        /* \001 is used to indicate boundaries between nested lists when there
         * is no blank line.  We split the string by \001 and parse
         * each chunk separately. */
        contents = strtok(elt.contents.str, "\001");
        res = markdown(contents, extensions);
        print_html_element(res, obfuscate);
        markdown_free(res);
        while ((contents = strtok(NULL, "\001"))) {
            res = markdown(contents, extensions);
            print_html_element(res, obfuscate);
            markdown_free(res);
        }
        break;
    case H1: case H2: case H3: case H4: case H5: case H6:
        lev = elt.key - H1 + 1;  /* assumes H1 ... H6 are in order */
        pad(2);
        fprintf(stream, "<h%1d>", lev);
        print_html_element_list(elt.children, obfuscate);
        fprintf(stream, "</h%1d>", lev);
        padded = 0;
        break;
    case PLAIN:
        pad(1);
        print_html_element_list(elt.children, obfuscate);
        padded = 0;
        break;
    case PARA:
        pad(2);
        fprintf(stream, "<p>");;;
        print_html_element_list(elt.children, obfuscate);
        fprintf(stream, "</p>");;;
        padded = 0;
        break;
    case HRULE:
        pad(2);
        fprintf(stream, "<hr />");;;
        padded = 0;
        break;
    case HTMLBLOCK:
        pad(2);
        fprintf(stream, "%s", elt.contents.str);
        padded = 0;
        break;
    case VERBATIM:
        pad(2);
        fprintf(stream, "%s", "<pre><code>");
        print_html_string(elt.contents.str, obfuscate);
        fprintf(stream, "%s", "</code></pre>");
        padded = 0;
        break;
    case BULLETLIST:
        pad(2);
        fprintf(stream, "%s", "<ul>");
        padded = 0;
        print_html_element_list(elt.children, obfuscate);
        pad(1);
        fprintf(stream, "%s", "</ul>");
        padded = 0;
        break;
    case ORDEREDLIST:
        pad(2);
        fprintf(stream, "%s", "<ol>");
        padded = 0;
        print_html_element_list(elt.children, obfuscate);
        pad(1);
        fprintf(stream, "</ol>");;;
        padded = 0;
        break;
    case LISTITEM:
        pad(1);
        fprintf(stream, "<li>");;;
        padded = 2;
        print_html_element_list(elt.children, obfuscate);
        fprintf(stream, "</li>");;;
        padded = 0;
        break;
    case BLOCKQUOTE:
        pad(2);
        fprintf(stream, "<blockquote>\n");;;
        padded = 2;
        print_html_element_list(elt.children, obfuscate);
        pad(1);
        fprintf(stream, "</blockquote>");;;
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
            fprintf(stream, "<a class=\"noteref\" id=\"fnref%d\" href=\"#fn%d\" title=\"Jump to note %d\">[%d]</a>",
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
    fprintf(stream, "<hr/>\n<ol id=\"notes\">");
    endnotes = reverse(endnotes);
    while (endnotes != NULL) {
        counter++;
        pad(1);
        fprintf(stream, "<li id=\"fn%d\">\n", counter);
        padded = 2;
        print_html_element_list(endnotes->children, false);
        fprintf(stream, " <a href=\"#fnref%d\" title=\"Jump back to reference\">[back]</a>", counter);
        pad(1);
        fprintf(stream, "</li>");;
        endnotes = endnotes->next;
    }
    pad(1);
    fprintf(stream, "</ol>");;;
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
            fprintf(stream, "\\%c", *str);
            break;
        case '^':
            fprintf(stream, "\\^{}");;;
            break;
        case '\\':
            fprintf(stream, "\\textbackslash{}");;;
            break;
        case '~':
            fprintf(stream, "\\ensuremath{\\sim}");;;
            break;
        case '|':
            fprintf(stream, "\\textbar{}");;;
            break;
        case '<':
            fprintf(stream, "\\textless{}");;;
            break;
        case '>':
            fprintf(stream, "\\textgreater{}");;;
            break;
        default:
            putc(*str, stream);
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
    element res;
    switch (elt.key) {
    case SPACE:
        fprintf(stream, "%s", elt.contents.str);
        break;
    case LINEBREAK:
        fprintf(stream, "\\\\\n");;;
        break;
    case STR:
        print_latex_string(elt.contents.str);
        break;
    case ELLIPSIS:
        fprintf(stream, "\\ldots{}");;;
        break;
    case EMDASH: 
        fprintf(stream, "---");;;
        break;
    case ENDASH: 
        fprintf(stream, "--");;;
        break;
    case APOSTROPHE:
        fprintf(stream, "'");;;
        break;
    case SINGLEQUOTED:
        fprintf(stream, "`");;;
        print_latex_element_list(elt.children);
        fprintf(stream, "'");;;
        break;
    case DOUBLEQUOTED:
        fprintf(stream, "``");;;
        print_latex_element_list(elt.children);
        fprintf(stream, "''");;;
        break;
    case CODE:
        fprintf(stream, "\\texttt{");;;
        print_latex_string(elt.contents.str);
        fprintf(stream, "}");;;
        break;
    case HTML:
        /* don't print HTML */
        break;
    case LINK:
        fprintf(stream, "\\href{%s}{", elt.contents.link.url);
        print_latex_element_list(elt.contents.link.label);
        fprintf(stream, "}");;;
        break;
    case IMAGE:
        fprintf(stream, "\\includegraphics{%s}", elt.contents.link.url);
        break;
    case EMPH:
        fprintf(stream, "\\emph{");;;
        print_latex_element_list(elt.children);
        fprintf(stream, "}");;;
        break;
    case STRONG:
        fprintf(stream, "\\textbf{");;;
        print_latex_element_list(elt.children);
        fprintf(stream, "}");;;
        break;
    case LIST:
        print_latex_element_list(elt.children);
        break;
    case RAW:
        /* \001 is used to indicate boundaries between nested lists when there
         * is no blank line.  We split the string by \001 and parse
         * each chunk separately. */
        contents = strtok(elt.contents.str, "\001");
        res = markdown(contents, extensions);
        print_latex_element(res);
        markdown_free(res);
        while ((contents = strtok(NULL, "\001"))) {
            res = markdown(contents, extensions);
            print_latex_element(res);
            markdown_free(res);
        }
        break;
    case H1: case H2: case H3:
        pad(2);
        lev = elt.key - H1 + 1;  /* assumes H1 ... H6 are in order */
        fprintf(stream, "\\");;;
        for (i = elt.key; i > H1; i--)
            fprintf(stream, "sub");;;
        fprintf(stream, "section{");;;
        print_latex_element_list(elt.children);
        fprintf(stream, "}");;;
        padded = 0;
        break;
    case H4: case H5: case H6:
        pad(2);
        fprintf(stream, "\\noindent\\textbf{");;;
        print_latex_element_list(elt.children);
        fprintf(stream, "}");;;
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
        fprintf(stream, "\\begin{center}\\rule{3in}{0.4pt}\\end{center}\n");;;
        padded = 0;
        break;
    case HTMLBLOCK:
        /* don't print HTML block */
        break;
    case VERBATIM:
        pad(1);
        fprintf(stream, "\\begin{verbatim}\n");;;
        print_latex_string(elt.contents.str);
        fprintf(stream, "\n\\end{verbatim}");;;
        padded = 0;
        break;
    case BULLETLIST:
        pad(1);
        fprintf(stream, "\\begin{itemize}");;;
        padded = 0;
        print_latex_element_list(elt.children);
        pad(1);
        fprintf(stream, "\\end{itemize}");;;
        padded = 0;
        break;
    case ORDEREDLIST:
        pad(1);
        fprintf(stream, "\\begin{enumerate}");;;
        padded = 0;
        print_latex_element_list(elt.children);
        pad(1);
        fprintf(stream, "\\end{enumerate}");;;
        padded = 0;
        break;
    case LISTITEM:
        pad(1);
        fprintf(stream, "\\item ");;;
        padded = 2;
        print_latex_element_list(elt.children);
        fprintf(stream, "\n");;;
        break;
    case BLOCKQUOTE:
        pad(1);
        fprintf(stream, "\\begin{quote}");;;
        padded = 0;
        res = markdown(elt.contents.str, extensions);
        print_latex_element(res);
        markdown_free(res);
        fprintf(stream, "\\end{quote}");;;
        padded = 0;
        break;
    case NOTE:
        /* if contents.str == 0, then print note; else ignore, since this
         * is a note block that has been incorporated into the notes list */
        if (elt.contents.str == 0) {
            fprintf(stream, "\\footnote{");;;
            padded = 2;
            print_latex_element_list(elt.children);
            fprintf(stream, "}");;;
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
            fprintf(stream, "\\e");;;
            break;
        default:
            putc(*str, stream);
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
    element res;
    switch (elt.key) {
    case SPACE:
        fprintf(stream, "%s", elt.contents.str);
        padded = 0;
        break;
    case LINEBREAK:
        pad(1);
        fprintf(stream, ".br");;;
        padded = 0;
        break;
    case STR:
        print_groff_string(elt.contents.str);
        padded = 0;
        break;
    case ELLIPSIS:
        fprintf(stream, "...");;;
        break;
    case EMDASH:
        fprintf(stream, "\\[em]");;;
        break;
    case ENDASH:
        fprintf(stream, "\\[en]");;;
        break;
    case APOSTROPHE:
        fprintf(stream, "'");;;
        break;
    case SINGLEQUOTED:
        fprintf(stream, "`");;;
        print_groff_mm_element_list(elt.children);
        fprintf(stream, "'");;;
        break;
    case DOUBLEQUOTED:
        fprintf(stream, "\\[lq]");;;
        print_groff_mm_element_list(elt.children);
        fprintf(stream, "\\[rq]");;;
        break;
    case CODE:
        fprintf(stream, "\\fC");;;
        print_groff_string(elt.contents.str);
        fprintf(stream, "\\fR");;;
        padded = 0;
        break;
    case HTML:
        /* don't print HTML */
        break;
    case LINK:
        print_groff_mm_element_list(elt.contents.link.label);
        fprintf(stream, " (%s)", elt.contents.link.url);
        padded = 0;
        break;
    case IMAGE:
        fprintf(stream, "[IMAGE: ");;;
        print_groff_mm_element_list(elt.contents.link.label);
        fprintf(stream, "]");;;
        padded = 0;
        /* not supported */
        break;
    case EMPH:
        fprintf(stream, "\\fI");;;
        print_groff_mm_element_list(elt.children);
        fprintf(stream, "\\fR");;;
        padded = 0;
        break;
    case STRONG:
        fprintf(stream, "\\fB");;;
        print_groff_mm_element_list(elt.children);
        fprintf(stream, "\\fR");;;
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
        res = markdown(contents, extensions);
        print_groff_mm_element(res, count);
        markdown_free(res);
        while ((contents = strtok(NULL, "\001"))) {
            res = markdown(contents, extensions);
            print_groff_mm_element(res, count);
            markdown_free(res);
        }
        break;
    case H1: case H2: case H3: case H4: case H5: case H6:
        lev = elt.key - H1 + 1;
        pad(1);
        fprintf(stream, ".H %d \"", lev);
        print_groff_mm_element_list(elt.children);
        fprintf(stream, "\"");
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
            fprintf(stream, ".P\n");;;
        print_groff_mm_element_list(elt.children);
        padded = 0;
        break;
    case HRULE:
        pad(1);
        fprintf(stream, "\\l'\\n(.lu*8u/10u'");;;
        padded = 0;
        break;
    case HTMLBLOCK:
        /* don't print HTML block */
        break;
    case VERBATIM:
        pad(1);
        fprintf(stream, ".VERBON 2\n");;;
        print_groff_string(elt.contents.str);
        fprintf(stream, ".VERBOFF");;;
        padded = 0;
        break;
    case BULLETLIST:
        pad(1);
        fprintf(stream, ".BL");;;
        padded = 0;
        print_groff_mm_element_list(elt.children);
        pad(1);
        fprintf(stream, ".LE 1");;;
        padded = 0;
        break;
    case ORDEREDLIST:
        pad(1);
        fprintf(stream, ".AL");;;
        padded = 0;
        print_groff_mm_element_list(elt.children);
        pad(1);
        fprintf(stream, ".LE 1");;;
        padded = 0;
        break;
    case LISTITEM:
        pad(1);
        fprintf(stream, ".LI\n");;;
        in_list_item = true;
        padded = 2;
        print_groff_mm_element_list(elt.children);
        in_list_item = false;
        break;
    case BLOCKQUOTE:
        pad(1);
        fprintf(stream, ".DS I\n");;;
        padded = 2;
        res = markdown(elt.contents.str, extensions);
        print_groff_mm_element(res, 1);
        markdown_free(res);
        pad(1);
        fprintf(stream, ".DE");;;
        padded = 0;
        break;
    case NOTE:
        /* if contents.str == 0, then print note; else ignore, since this
         * is a note block that has been incorporated into the notes list */
        if (elt.contents.str == 0) {
            fprintf(stream, "\\*F\n");;;
            fprintf(stream, ".FS\n");;;
            padded = 2;
            print_groff_mm_element_list(elt.children);
            pad(1);
            fprintf(stream, ".FE\n");;;
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

void print_element(element elt, FILE * out, int format, int exts) {
    stream = out;
    extensions = exts;
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
