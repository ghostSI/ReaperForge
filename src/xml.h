#ifndef XML_H
#define XML_H

#include <stdint.h>
#include <stddef.h>

#if defined(_MSC_VER) && !defined(__cplusplus) && !defined(inline)
#define inline __inline
#endif

typedef enum {
    YXML_EEOF        = -5, /* Unexpected EOF                             */
    YXML_EREF        = -4, /* Invalid character or entity reference (&whatever;) */
    YXML_ECLOSE      = -3, /* Close tag does not match open tag (<Tag> .. </OtherTag>) */
    YXML_ESTACK      = -2, /* Stack overflow (too deeply nested tags or too long element/attribute name) */
    YXML_ESYN        = -1, /* Syntax error (unexpected byte)             */
    YXML_OK          =  0, /* Character consumed, no new token present   */
    YXML_ELEMSTART   =  1, /* Start of an element:   '<Tag ..'           */
    YXML_CONTENT     =  2, /* Element content                            */
    YXML_ELEMEND     =  3, /* End of an element:     '.. />' or '</Tag>' */
    YXML_ATTRSTART   =  4, /* Attribute:             'Name=..'           */
    YXML_ATTRVAL     =  5, /* Attribute value                            */
    YXML_ATTREND     =  6, /* End of attribute       '.."'               */
    YXML_PISTART     =  7, /* Start of a processing instruction          */
    YXML_PICONTENT   =  8, /* Content of a PI                            */
    YXML_PIEND       =  9  /* End of a processing instruction            */
} yxml_ret_t;

typedef struct {
    char *elem;

    char data[8];

    char *attr;

    char *pi;

    uint64_t byte;
    uint64_t total;
    uint32_t line;


    int state;
    unsigned char *stack;
    size_t stacksize, stacklen;
    unsigned reflen;
    unsigned quote;
    int nextstate;
    unsigned ignore;
    unsigned char *string;
} yxml_t;


#ifdef __cplusplus
extern "C" {
#endif

void yxml_init(yxml_t *, void *, size_t);


yxml_ret_t yxml_parse(yxml_t *, int);

yxml_ret_t yxml_eof(yxml_t *);

#ifdef __cplusplus
}
#endif

static inline size_t yxml_symlen(yxml_t *x, const char *s) {
    return (x->stack + x->stacklen) - (const unsigned char*)s;
}

#endif
