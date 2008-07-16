#include <stdlib.h>
#include <stdio.h>
#include <glib.h>

enum markdown_extensions {
    EXT_SMART            = 1,
    EXT_NOTES            = 2,
    EXT_FILTER_HTML      = 3,
    EXT_FILTER_STYLES    = 4
};

enum markdown_formats {
    HTML_FORMAT,
    LATEX_FORMAT,
    GROFF_MM_FORMAT
};

GString * markdown_to_g_string(char *text, int extensions, int output_format);
char * markdown_to_string(char *text, int extensions, int output_format);

/* vim: set ts=4 sw=4 : */
