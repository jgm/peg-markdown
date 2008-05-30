#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "bufopen.h"
#include "markdown_peg.h"

#define TABSTOP 4
#define INCREMENT 4096  /* size of chunks in which to allocate memory */


/* preformat_text - allocate and copy text buffer while
 * performing tab expansion. */
static char *preformat_text(char *text) {
    char *buf, *p;
    int charstotab;

    int len = 0;
    int maxlen = INCREMENT;

    buf = malloc(maxlen);
    p = buf;

    charstotab = TABSTOP;
    while ((*p = *text++) != '\0') {
        switch (*p) {
        case '\t':
            while (charstotab > 0)
                *p = ' ', p++, len++, charstotab--;
            break;
        case '\n':
            p++, len++, charstotab = TABSTOP;
            break;
        default:
            p++, len++, charstotab--;
        }
        if (charstotab == 0)
            charstotab = TABSTOP;
        if (len > maxlen - TABSTOP - 4) {
            maxlen += INCREMENT;
            buf = realloc(buf, maxlen);
            assert(buf != NULL);
            p = buf + len;
        }
    }
    assert(*p == '\0');
    strcat(p, "\n\n");
    return buf;
}


static void print_tree(element * elt, int indent) {
    int i;
    char * key;
    while (elt != NULL) {
        for (i = 0; i < indent; i++)
            fputc(' ', stderr);
        switch (elt->key) {
            case LIST:               key = "LIST"; break;
            case RAW:                key = "RAW"; break;
            case SPACE:              key = "SPACE"; break;
            case LINEBREAK:          key = "LINEBREAK"; break;
            case ELLIPSIS:           key = "ELLIPSIS"; break;
            case EMDASH:             key = "EMDASH"; break;
            case ENDASH:             key = "ENDASH"; break;
            case APOSTROPHE:         key = "APOSTROPHE"; break;
            case SINGLEQUOTED:       key = "SINGLEQUOTED"; break;
            case DOUBLEQUOTED:       key = "DOUBLEQUOTED"; break;
            case STR:                key = "STR"; break;
            case LINK:               key = "LINK"; break;
            case IMAGE:              key = "IMAGE"; break;
            case CODE:               key = "CODE"; break;
            case HTML:               key = "HTML"; break;
            case EMPH:               key = "EMPH"; break;
            case STRONG:             key = "STRONG"; break;
            case PLAIN:              key = "PLAIN"; break;
            case PARA:               key = "PARA"; break;
            case LISTITEM:           key = "LISTITEM"; break;
            case BULLETLIST:         key = "BULLETLIST"; break;
            case ORDEREDLIST:        key = "ORDEREDLIST"; break;
            case H1:                 key = "H1"; break;
            case H2:                 key = "H2"; break;
            case H3:                 key = "H3"; break;
            case H4:                 key = "H4"; break;
            case H5:                 key = "H5"; break;
            case H6:                 key = "H6"; break;
            case BLOCKQUOTE:         key = "BLOCKQUOTE"; break;
            case VERBATIM:           key = "VERBATIM"; break;
            case HTMLBLOCK:          key = "HTMLBLOCK"; break;
            case HRULE:              key = "HRULE"; break;
            case REFERENCE:          key = "REFERENCE"; break;
            case NOTE:               key = "NOTE"; break;
            default:                 key = "?";
        }
        if ( elt->key == STR ) {
            fprintf(stderr, "0x%x: %s   '%s'\n", (int)elt, key, elt->contents.str);
        } else {
            fprintf(stderr, "0x%x: %s\n", (int)elt, key);
        }
        if (elt->children)
            print_tree(elt->children, indent + 4);
        elt = elt->next;
    }
}

/* markdown_to_stream = convert markdown text to the output format specified
 * and write output to the specified stream. */
int markdown_to_stream(char *text, int extensions, int output_format, FILE *stream) {
    element result;
    char *formatted_text;

    if (stream == NULL)
        return -1;

    formatted_text = preformat_text(text);
    result = markdown(formatted_text, extensions);
    free(formatted_text);

    print_element(result, stream, output_format, extensions);
    fflush(stream);

    markdown_free(result);

    return 0;
}

/* markdown_to_string = convert markdown text to the output format specified
 * and return a null terminated string. The value returned is allocated using
 * malloc and should be freed with free. */
char * markdown_to_string(char *text, int extensions, int output_format) {
    FILE *stream;

    /* buf and buflen are set _after_ fclose is called on stream */
    char *buf = NULL;
    size_t buflen = 0;
    if ((stream = bufopen(0, &buf, &buflen)) == NULL)
        return NULL;

    markdown_to_stream(text, extensions, output_format, stream);
    fclose(stream);

    return buf;
}


/* vim:set ts=4 sw=4: */
