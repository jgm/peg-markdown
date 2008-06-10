#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "markdown_peg.h"

#define TABSTOP 4

/* preformat_text - allocate and copy text buffer while
 * performing tab expansion. */
static char *preformat_text(char *text) {
    GString *buf;
    char next_char;
    int charstotab;

    int len = 0;

    buf = g_string_new("");

    charstotab = TABSTOP;
    while ((next_char = *text++) != '\0') {
        switch (next_char) {
        case '\t':
            while (charstotab > 0)
                g_string_append_c(buf, ' '), len++, charstotab--;
            break;
        case '\n':
            g_string_append_c(buf, '\n'), len++, charstotab = TABSTOP;
            break;
        default:
            g_string_append_c(buf, next_char), len++, charstotab--;
        }
        if (charstotab == 0)
            charstotab = TABSTOP;
    }
    g_string_append(buf, "\n\n");
    return(buf->str);
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

static element * process_raw_blocks(element *input, int extensions, element *references, element *notes);

/* markdown_to_gstring = convert markdown text to the output format specified
 * and return a GString. */
GString * markdown_to_g_string(char *text, int extensions, int output_format) {
    element *result;
    element *references;
    element *notes;
    char *formatted_text;
    GString *out;
    out = g_string_new("");

    formatted_text = preformat_text(text);

    references = parse_references(formatted_text, extensions);
    notes = parse_notes(formatted_text, extensions, references);
    result = parse_markdown(formatted_text, extensions, references, notes);
    result = process_raw_blocks(result, extensions, references, notes);

    free(formatted_text);

    print_element_list(out, result, output_format, extensions);

    free_element_list(result);
    free_element_list(references);
    free_element_list(notes);
    return out;
}

static element * process_raw_blocks(element *input, int extensions, element *references, element *notes) {
    element *current = NULL;
    element *last_child = NULL;
    char *contents;
    current = input;

    while (current != NULL) {
        if (current->key == RAW) {
            /* \001 is used to indicate boundaries between nested lists when there
             * is no blank line.  We split the string by \001 and parse
             * each chunk separately. */
            contents = strtok(current->contents.str, "\001");
            current->key = LIST;
            current->children = parse_markdown(contents, extensions, references, notes);
            last_child = current->children;
            while ((contents = strtok(NULL, "\001"))) {
                while (last_child->next != NULL)
                    last_child = last_child->next;
                last_child->next = parse_markdown(contents, extensions, references, notes);
            free(current->contents.str);
            current->contents.str = NULL;
            }
        }
        if (current->children != NULL)
            current->children = process_raw_blocks(current->children, extensions, references, notes);
        current = current->next;
    }
    return input;
}

/* markdown_to_string = convert markdown text to the output format specified
 * and return a null-terminated string. */
char * markdown_to_string(char *text, int extensions, int output_format) {
    GString *out;
    char *char_out;
    out = markdown_to_g_string(text, extensions, output_format);
    char_out = strdup(out->str);
    g_string_free(out, true);
    return char_out;
}

/* markdown_to_stream = convert markdown text to the output format specified
 * and write output to the specified stream. */
int markdown_to_stream(char *text, int extensions, int output_format, FILE *stream) {
    char *out;
    out = markdown_to_string(text, extensions, output_format);
    fprintf(stream, "%s", out);
    free(out);
    return 0;
}

/* vim:set ts=4 sw=4: */
