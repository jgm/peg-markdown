/* markdown_peg.h */

extern char *strdup(const char *string);

/**********************************************************************

  Data Structures

 ***********************************************************************/

/* Information (label, URL and title) for a link. */
struct Link {
    struct Element   *label;
    char             *url;
    char             *title;    
};

typedef struct Link link;

/* Union for contents of an Element (string, list, or link). */
union Contents {
    char             *str;
    struct Link      link;
};

/* Types of semantic values returned by parsers. */ 
enum keys { LIST,   /* A generic list of values.  For ordered and bullet lists, see below. */
            SPACE,
            LINEBREAK,
            ELLIPSIS,
            EMDASH,
            ENDASH,
            STR,
            LINK,
            IMAGE,
            CODE,
            HTML,
            EMPH,
            STRONG,
            PLAIN,
            PARA,
            LISTITEM,
            BULLETLIST,
            ORDEREDLIST,
            H1, H2, H3, H4, H5, H6,  /* Code assumes that these are in order. */
            BLOCKQUOTE,
            VERBATIM,
            HTMLBLOCK,
            HRULE,
            REFERENCE
          };

/* Semantic value of a parsing action. */
struct Element {
    int               key;
    union Contents    contents;
    struct Element    *children;
    struct Element    *next;
};

typedef struct Element element;

enum markdown_extensions { 
    EXT_SMART            = 1
};  

element markdown(char *string, int extensions);

/* Output formats. */
enum formats { HTML_FORMAT,
               LATEX_FORMAT,
               GROFF_MM_FORMAT
             };

void print_html_string(char *str, bool obfuscate);
void print_html_element(element elt, bool obfuscate);
void print_html_element_list(element *list, bool obfuscate);
void print_latex_string(char *str);
void print_latex_element(element elt);
void print_latex_element_list(element *list);
void print_latex_element(element elt);
void print_groff_string(char *str);
void print_groff_mm_element(element elt, int count);
void print_groff_mm_element_list(element *list);
void print_groff_mm_element(element elt, int count);
void print_element(element elt, int format);

