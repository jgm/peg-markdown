/* markdown_peg.h */

extern char *strdup(const char *string);

/**********************************************************************

  Data Structures

 ***********************************************************************/

/* Information (label, URL and title) for a link. */
struct Link {
    struct ElementListItem   *label;
    char                     *url;
    char                     *title;    
};

typedef struct Link link;

/* Union for contents of an Element (string, list, or link). */
union Contents {
    char                   *str;
    struct ElementListItem *list;
    struct Link            link;
};

/* Types of semantic values returned by parsers. */ 
enum keys { LIST,   /* A generic list of values.  For ordered and bullet lists, see below. */
            SPACE,
            LINEBREAK,
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

/* Output formats. */
enum formats { HTML_FORMAT,
               LATEX_FORMAT
             };

/* Semantic value of a parsing action. */
struct Element {
    int            key;
    union Contents contents;
};

typedef struct Element element;

/* Node in linked list of Elements. */
struct ElementListItem {
    element                 val;
    struct ElementListItem  *next;    
};

typedef struct ElementListItem item;

element markdown(char *string);

