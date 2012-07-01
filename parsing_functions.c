/* parsing_functions.c - Functions for parsing markdown and
 * freeing element lists. */

/* int yyparse(void); */

static void free_element_contents(element elt);

/* free_element_list - free list of elements recursively */
void free_element_list(element * elt) {
    element * next = NULL;
    while (elt != NULL) {
        next = elt->next;
        free_element_contents(*elt);
        if (elt->children != NULL) {
            free_element_list(elt->children);
            elt->children = NULL;
        }
        free(elt);
        elt = next;
    }
}

/* free_element_contents - free element contents depending on type */
static void free_element_contents(element elt) {
    switch (elt.key) {
      case STR:
      case SPACE:
      case RAW:
      case HTMLBLOCK:
      case HTML:
      case VERBATIM:
      case CODE:
      case NOTE:
        free(elt.contents.str);
        elt.contents.str = NULL;
        break;
      case LINK:
      case IMAGE:
      case REFERENCE:
        free(elt.contents.link->url);
        elt.contents.link->url = NULL;
        free(elt.contents.link->title);
        elt.contents.link->title = NULL;
        free_element_list(elt.contents.link->label);
        free(elt.contents.link);
        elt.contents.link = NULL;
        break;
      default:
        ;
    }
}

/* free_element - free element and contents */
void free_element(element *elt) {
    free_element_contents(*elt);
    free(elt);
}

element * parse_references(char *string, int extensions) {

    char *oldcharbuf;
    syntax_extensions = extensions;

    oldcharbuf = charbuf;
    charbuf = string;
    yyparsefrom(yy_References);    /* first pass, just to collect references */
    charbuf = oldcharbuf;

    return references;
}

element * parse_notes(char *string, int extensions, element *reference_list) {

    char *oldcharbuf;
    notes = NULL;
    syntax_extensions = extensions;

    if (extension(EXT_NOTES)) {
        references = reference_list;
        oldcharbuf = charbuf;
        charbuf = string;
        yyparsefrom(yy_Notes);     /* second pass for notes */
        charbuf = oldcharbuf;
    }

    return notes;
}

element * parse_markdown(char *string, int extensions, element *reference_list, element *note_list) {

    char *oldcharbuf;
    syntax_extensions = extensions;
    references = reference_list;
    notes = note_list;

    oldcharbuf = charbuf;
    charbuf = string;

    yyparsefrom(yy_Doc);

    charbuf = oldcharbuf;          /* restore charbuf to original value */
    return parse_result;

}
