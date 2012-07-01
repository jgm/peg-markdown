#include <stdlib.h>
#include <stdio.h>
#include <glib.h>

enum markdown_extensions {
    EXT_SMART            = 0x01,
    EXT_NOTES            = 0x02,
    EXT_FILTER_HTML      = 0x04,
    EXT_FILTER_STYLES    = 0x08
};

enum markdown_formats {
    HTML_FORMAT = 0,
    LATEX_FORMAT = 1,
    GROFF_MM_FORMAT = 2,
    ODF_FORMAT = 3
};

GString * markdown_to_g_string(char *text, int extensions, int output_format);
char * markdown_to_string(char *text, int extensions, int output_format);

/* vim: set ts=4 sw=4 : */
