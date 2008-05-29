#include <stdlib.h>
#include <stdio.h>

enum markdown_extensions {
    EXT_SMART            = 1,
    EXT_NOTES            = 2
};

enum markdown_formats {
    HTML_FORMAT,
    LATEX_FORMAT,
    GROFF_MM_FORMAT
};

int    markdown_to_stream(char *text, int extensions, int output_format, FILE *stream);
char * markdown_to_string(char *text, int extensions, int output_format);

/* vim: set ts=4 sw=4 : */
