/*
/* xml lexer */
#ifndef XML_LEXER_H
#define XML_LEXER_H

/* public constants */
#define T_ERROR         -1   /* lexer error */
#define T_EOF            0   /* end of file */
#define T_EOL            1   /* end of line */
#define T_SEPAR          2   /* separator ' ' '/t' '\n' '\r' */
#define T_M_START_1      3   /* markup start < */
#define T_M_START_2      4   /* markup start </ */
#define T_M_STOP_1       5   /* markup stop > */
#define T_M_STOP_2       6   /* markup stop /> */
#define T_EQUAL          7   /* = */
#define T_QUOTE          8   /* \" or \' */
#define T_STRING         9   /* "string" */
#define T_IDENT         10   /* identifier */
#define T_DATA          11   /* data */
#define T_C_START       12   /* <!-- */
#define T_C_STOP        13   /* --> */
#define T_TI_START      14   /* <? */
#define T_TI_STOP       15   /* ?> */
#define T_DOCTYPE_START 16   /* <!DOCTYPE */
#define T_DOCTYPE_STOP  17   /* > */


/* public functions */
void lexer_init(char * buf, int size);
int lexer_get_token(char * tok, int tok_size);

#endif
