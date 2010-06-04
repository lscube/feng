/* Driver template for the LEMON parser generator.
** The author disclaims copyright to this source code.
*/
/* First off, code is include which follows the "include" declaration
** in the input file. */
#include <stdio.h>

#include <glib.h>

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <config.h>
#include "configfile.h"
#include "buffer.h"
#include "array.h"

static void configparser_push(config_t *ctx, data_config *dc, int isnew) {
  if (isnew) {
    dc->context_ndx = ctx->all_configs->used;
    assert(dc->context_ndx > ctx->current->context_ndx);
    array_insert_unique(ctx->all_configs, (data_unset *)dc);
    dc->parent = ctx->current;
    array_insert_unique(dc->parent->childs, (data_unset *)dc);
  }
  array_insert_unique(ctx->configs_stack, (data_unset *)ctx->current);
  ctx->current = dc;
}

static data_config *configparser_pop(config_t *ctx) {
  data_config *old = ctx->current;
  ctx->current = (data_config *) array_pop(ctx->configs_stack);
  return old;
}

/* return a copied variable */
static data_unset *configparser_get_variable(config_t *ctx, const conf_buffer *key) {
  data_unset *du;
  data_config *dc;

  for (dc = ctx->current; dc; dc = dc->parent) {
    if (NULL != (du = array_get_element(dc->value, key->ptr))) {
      return du->copy(du);
    }
  }
  return NULL;
}

/* op1 is to be eat/return by this function, op1->key is not cared
   op2 is left untouch, unreferenced
 */
static data_unset *configparser_merge_data(data_unset *op1, const data_unset *op2) {
  /* type mismatch */
  if (op1->type != op2->type) {
    if (op1->type == TYPE_STRING && op2->type == TYPE_INTEGER) {
      data_string *ds = (data_string *)op1;
      buffer_append_long(ds->value, ((data_integer*)op2)->value);
      return op1;
    } else if (op1->type == TYPE_INTEGER && op2->type == TYPE_STRING) {
      data_string *ds = data_string_init();
      buffer_append_long(ds->value, ((data_integer*)op1)->value);
      buffer_append_string_buffer(ds->value, ((data_string*)op2)->value);
      op1->free(op1);
      return (data_unset *)ds;
    } else {
      fprintf(stderr, "data type mismatch, cannot be merge\n");
      op1->free(op1);
      return NULL;
    }
  }

  switch (op1->type) {
    case TYPE_STRING:
      buffer_append_string_buffer(((data_string *)op1)->value, ((data_string *)op2)->value);
      break;
    case TYPE_INTEGER:
      ((data_integer *)op1)->value += ((data_integer *)op2)->value;
      break;
    case TYPE_ARRAY: {
      array *dst = ((data_array *)op1)->value;
      array *src = ((data_array *)op2)->value;
      data_unset *du;
      size_t i;

      for (i = 0; i < src->used; i ++) {
        du = (data_unset *)src->data[i];
        if (du) {
          array_insert_unique(dst, du->copy(du));
        }
      }
      break;
    default:
      assert(0);
      break;
    }
  }
  return op1;
}


/* Next is all token values, in a form suitable for use by makeheaders.
** This section will be null unless lemon is run with the -m switch.
*/
/*
** These constants (all generated automatically by the parser generator)
** specify the various kinds of tokens (terminals) that the parser
** understands.
**
** Each symbol here is a terminal symbol in the grammar.
*/
/* Make sure the INTERFACE macro is defined.
*/
#ifndef INTERFACE
# define INTERFACE 1
#endif
/* The next thing included is series of defines which control
** various aspects of the generated parser.
**    YYCODETYPE         is the data type used for storing terminal
**                       and nonterminal numbers.  "unsigned char" is
**                       used if there are fewer than 250 terminals
**                       and nonterminals.  "int" is used otherwise.
**    YYNOCODE           is a number of type YYCODETYPE which corresponds
**                       to no legal terminal or nonterminal number.  This
**                       number is used to fill in empty slots of the hash
**                       table.
**    YYFALLBACK         If defined, this indicates that one or more tokens
**                       have fall-back values which should be used if the
**                       original value of the token will not parse.
**    YYACTIONTYPE       is the data type used for storing terminal
**                       and nonterminal numbers.  "unsigned char" is
**                       used if there are fewer than 250 rules and
**                       states combined.  "int" is used otherwise.
**    configparserTOKENTYPE     is the data type used for minor tokens given
**                       directly to the parser from the tokenizer.
**    YYMINORTYPE        is the data type used for all minor tokens.
**                       This is typically a union of many types, one of
**                       which is configparserTOKENTYPE.  The entry in the union
**                       for base tokens is called "yy0".
**    YYSTACKDEPTH       is the maximum depth of the parser's stack.
**    configparserARG_SDECL     A static variable declaration for the %extra_argument
**    configparserARG_PDECL     A parameter declaration for the %extra_argument
**    configparserARG_STORE     Code to store %extra_argument into yypParser
**    configparserARG_FETCH     Code to extract %extra_argument from yypParser
**    YYNSTATE           the combined number of states.
**    YYNRULE            the number of rules in the grammar
**    YYERRORSYMBOL      is the code number of the error symbol.  If not
**                       defined, then do no error processing.
*/
/*  */
#define YYCODETYPE unsigned char
#define YYNOCODE 48
#define YYACTIONTYPE unsigned char
#define configparserTOKENTYPE conf_buffer *
typedef union {
  configparserTOKENTYPE yy0;
  config_cond_t yy27;
  array * yy40;
  data_unset * yy41;
  conf_buffer * yy43;
  data_config * yy78;
  int yy95;
} YYMINORTYPE;
#define YYSTACKDEPTH 100
#define configparserARG_SDECL config_t *ctx;
#define configparserARG_PDECL ,config_t *ctx
#define configparserARG_FETCH config_t *ctx = yypParser->ctx
#define configparserARG_STORE yypParser->ctx = ctx
#define YYNSTATE 63
#define YYNRULE 40
#define YYERRORSYMBOL 26
#define YYERRSYMDT yy95
#define YY_NO_ACTION      (YYNSTATE+YYNRULE+2)
#define YY_ACCEPT_ACTION  (YYNSTATE+YYNRULE+1)
#define YY_ERROR_ACTION   (YYNSTATE+YYNRULE)

/* Next are that tables used to determine what action to take based on the
** current state and lookahead token.  These tables are used to implement
** functions that take a state number and lookahead value and return an
** action integer.
**
** Suppose the action integer is N.  Then the action is determined as
** follows
**
**   0 <= N < YYNSTATE                  Shift N.  That is, push the lookahead
**                                      token onto the stack and goto state N.
**
**   YYNSTATE <= N < YYNSTATE+YYNRULE   Reduce by rule N-YYNSTATE.
**
**   N == YYNSTATE+YYNRULE              A syntax error has occurred.
**
**   N == YYNSTATE+YYNRULE+1            The parser accepts its input.
**
**   N == YYNSTATE+YYNRULE+2            No such action.  Denotes unused
**                                      slots in the yy_action[] table.
**
** The action table is constructed as a single large table named yy_action[].
** Given state S and lookahead X, the action is computed as
**
**      yy_action[ yy_shift_ofst[S] + X ]
**
** If the index value yy_shift_ofst[S]+X is out of range or if the value
** yy_lookahead[yy_shift_ofst[S]+X] is not equal to X or if yy_shift_ofst[S]
** is equal to YY_SHIFT_USE_DFLT, it means that the action is not in the table
** and that yy_default[S] should be used instead.
**
** The formula above is for computing the action when the lookahead is
** a terminal symbol.  If the lookahead is a non-terminal (as occurs after
** a reduce action) then the yy_reduce_ofst[] array is used in place of
** the yy_shift_ofst[] array and YY_REDUCE_USE_DFLT is used in place of
** YY_SHIFT_USE_DFLT.
**
** The following are the tables generated in this section:
**
**  yy_action[]        A single table containing all actions.
**  yy_lookahead[]     A table containing the lookahead for each entry in
**                     yy_action.  Used to detect hash collisions.
**  yy_shift_ofst[]    For each state, the offset into yy_action for
**                     shifting terminals.
**  yy_reduce_ofst[]   For each state, the offset into yy_action for
**                     shifting non-terminals after a reduce.
**  yy_default[]       Default action for each state.
*/
static YYACTIONTYPE yy_action[] = {
 /*     0 */     2,    3,    4,    5,   13,   14,   63,   15,    7,   45,
 /*    10 */    20,   88,   16,   46,   28,   49,   41,   10,   40,   25,
 /*    20 */    22,   50,   46,    8,   15,  104,    1,   20,   28,   18,
 /*    30 */    58,   60,    6,   25,   22,   40,   47,   62,   11,   46,
 /*    40 */    20,    9,   23,   24,   26,   29,   89,   58,   60,   10,
 /*    50 */    17,   38,   28,   27,   37,   19,   30,   25,   22,   34,
 /*    60 */    15,  100,   20,   20,   23,   24,   26,   12,   19,   31,
 /*    70 */    32,   40,   19,   44,   43,   46,   95,   35,   90,   89,
 /*    80 */    28,   49,   42,   58,   60,   25,   22,   59,   28,   27,
 /*    90 */    33,   48,   52,   25,   22,   34,   28,   49,   51,   28,
 /*   100 */    36,   25,   22,   61,   25,   22,   89,   28,   39,   89,
 /*   110 */    89,   89,   25,   22,   54,   55,   56,   57,   89,   28,
 /*   120 */    53,   21,   89,   89,   25,   22,   25,   22,
};
static YYCODETYPE yy_lookahead[] = {
 /*     0 */    29,   30,   31,   32,   33,   34,    0,    1,   44,   38,
 /*    10 */     4,   15,   41,   16,   35,   36,   45,   46,   12,   40,
 /*    20 */    41,   42,   16,   15,    1,   27,   28,    4,   35,   36,
 /*    30 */    24,   25,    1,   40,   41,   12,   17,   14,   13,   16,
 /*    40 */     4,   38,    6,    7,    8,    9,   15,   24,   25,   46,
 /*    50 */     2,    3,   35,   36,   37,    5,   39,   40,   41,   42,
 /*    60 */     1,   11,    4,    4,    6,    7,    8,   28,    5,    9,
 /*    70 */    10,   12,    5,   14,   28,   16,   13,   11,   13,   47,
 /*    80 */    35,   36,   13,   24,   25,   40,   41,   42,   35,   36,
 /*    90 */    37,   18,   43,   40,   41,   42,   35,   36,   19,   35,
 /*   100 */    36,   40,   41,   42,   40,   41,   47,   35,   36,   47,
 /*   110 */    47,   47,   40,   41,   20,   21,   22,   23,   47,   35,
 /*   120 */    36,   35,   47,   47,   40,   41,   40,   41,
};
#define YY_SHIFT_USE_DFLT (-5)
static signed char yy_shift_ofst[] = {
 /*     0 */    -5,    6,   -5,   -5,   -5,   31,   -4,    8,   -3,   -5,
 /*    10 */    25,   -5,   23,   -5,   -5,   -5,   48,   58,   67,   58,
 /*    20 */    -5,   -5,   -5,   -5,   -5,   -5,   36,   50,   -5,   -5,
 /*    30 */    60,   -5,   58,   -5,   66,   58,   67,   -5,   58,   67,
 /*    40 */    65,   69,   -5,   59,   -5,   -5,   19,   73,   58,   67,
 /*    50 */    79,   94,   58,   63,   -5,   -5,   -5,   -5,   58,   -5,
 /*    60 */    58,   -5,   -5,
};
#define YY_REDUCE_USE_DFLT (-37)
static signed char yy_reduce_ofst[] = {
 /*     0 */    -2,  -29,  -37,  -37,  -37,  -36,  -37,  -37,    3,  -37,
 /*    10 */   -37,   39,  -29,  -37,  -37,  -37,  -37,   -7,  -37,   86,
 /*    20 */   -37,  -37,  -37,  -37,  -37,  -37,   17,  -37,  -37,  -37,
 /*    30 */   -37,  -37,   53,  -37,  -37,   64,  -37,  -37,   72,  -37,
 /*    40 */   -37,  -37,   46,  -29,  -37,  -37,  -37,  -37,  -21,  -37,
 /*    50 */   -37,   49,   84,  -37,  -37,  -37,  -37,  -37,   45,  -37,
 /*    60 */    61,  -37,  -37,
};
static YYACTIONTYPE yy_default[] = {
 /*     0 */    65,  103,   64,   66,   67,  103,   68,  103,  103,   92,
 /*    10 */   103,   65,  103,   69,   70,   71,  103,  103,   72,  103,
 /*    20 */    74,   75,   77,   78,   79,   80,  103,   86,   76,   81,
 /*    30 */   103,   82,   84,   83,  103,  103,   87,   85,  103,   73,
 /*    40 */   103,  103,   65,  103,   91,   93,  103,  103,  103,  100,
 /*    50 */   103,  103,  103,  103,   96,   97,   98,   99,  103,  101,
 /*    60 */   103,  102,   94,
};
#define YY_SZ_ACTTAB (sizeof(yy_action)/sizeof(yy_action[0]))

/* The next table maps tokens into fallback tokens.  If a construct
** like the following:
**
**      %fallback ID X Y Z.
**
** appears in the grammer, then ID becomes a fallback token for X, Y,
** and Z.  Whenever one of the tokens X, Y, or Z is input to the parser
** but it does not parse, the type of the token is changed to ID and
** the parse is retried before an error is thrown.
*/

/* The following structure represents a single element of the
** parser's stack.  Information stored includes:
**
**   +  The state number for the parser at this level of the stack.
**
**   +  The value of the token stored at this level of the stack.
**      (In other words, the "major" token.)
**
**   +  The semantic value stored at this level of the stack.  This is
**      the information used by the action routines in the grammar.
**      It is sometimes called the "minor" token.
*/
struct yyStackEntry {
  int stateno;       /* The state-number */
  int major;         /* The major token value.  This is the code
                     ** number for the token at this stack level */
  YYMINORTYPE minor; /* The user-supplied minor token value.  This
                     ** is the value of the token  */
};
typedef struct yyStackEntry yyStackEntry;

/* The state of the parser is completely contained in an instance of
** the following structure */
struct yyParser {
  int yyidx;                    /* Index of top element in stack */
  int yyerrcnt;                 /* Shifts left before out of the error */
  configparserARG_SDECL                /* A place to hold %extra_argument */
  yyStackEntry yystack[YYSTACKDEPTH];  /* The parser's stack */
};
typedef struct yyParser yyParser;

/*
** This function allocates a new parser.
** The only argument is a pointer to a function which works like
** malloc.
**
** Inputs:
** A pointer to the function used to allocate memory.
**
** Outputs:
** A pointer to a parser.  This pointer is used in subsequent calls
** to configparser and configparserFree.
*/
void *configparserAlloc(){
    yyParser *pParser = g_malloc0(sizeof(yyParser));
    return pParser;
}

/* The following function deletes the value associated with a
** symbol.  The symbol can be either a terminal or nonterminal.
** "yymajor" is the symbol code, and "yypminor" is a pointer to
** the value.
*/
static void yy_destructor(YYCODETYPE yymajor, YYMINORTYPE *yypminor){
  switch( yymajor ){
    /* Here is inserted the actions which take place when a
    ** terminal or non-terminal is destroyed.  This can happen
    ** when the symbol is popped from the stack during a
    ** reduce or during error processing or when a parser is
    ** being destroyed before it is finished parsing.
    **
    ** Note: during a reduce, the only symbols destroyed are those
    ** which appear on the RHS of the rule, but which are not used
    ** inside the C code.
    */
    case 1:
    case 2:
    case 3:
    case 4:
    case 5:
    case 6:
    case 7:
    case 8:
    case 9:
    case 10:
    case 11:
    case 12:
    case 13:
    case 14:
    case 15:
    case 16:
    case 17:
    case 18:
    case 19:
    case 20:
    case 21:
    case 22:
    case 23:
    case 24:
    case 25:
{ buffer_free((yypminor->yy0)); }
      break;
    case 35:
{ (yypminor->yy41)->free((yypminor->yy41)); }
      break;
    case 36:
{ (yypminor->yy41)->free((yypminor->yy41)); }
      break;
    case 37:
{ (yypminor->yy41)->free((yypminor->yy41)); }
      break;
    case 39:
{ array_free((yypminor->yy40)); }
      break;
    case 40:
{ array_free((yypminor->yy40)); }
      break;
    case 41:
{ buffer_free((yypminor->yy43)); }
      break;
    case 42:
{ buffer_free((yypminor->yy43)); }
      break;
    default:  break;   /* If no destructor action specified: do nothing */
  }
}

/*
** Pop the parser's stack once.
**
** If there is a destructor routine associated with the token which
** is popped from the stack, then call it.
**
** Return the major token number for the symbol popped.
*/
static int yy_pop_parser_stack(yyParser *pParser){
  YYCODETYPE yymajor;
  yyStackEntry *yytos = &pParser->yystack[pParser->yyidx];

  if( pParser->yyidx<0 ) return 0;
  yymajor = yytos->major;
  yy_destructor( yymajor, &yytos->minor);
  pParser->yyidx--;
  return yymajor;
}

/*
** Deallocate and destroy a parser.  Destructors are all called for
** all stack elements before shutting the parser down.
**
** Inputs:
** <ul>
** <li>  A pointer to the parser.  This should be a pointer
**       obtained from configparserAlloc.
** <li>  A pointer to a function used to reclaim memory obtained
**       from malloc.
** </ul>
*/
void configparserFree(
  void *p                    /* The parser to be deleted */
){
  yyParser *pParser = (yyParser*)p;
  if( pParser==0 ) return;
  while( pParser->yyidx>=0 ) yy_pop_parser_stack(pParser);
  g_free(pParser);
}

/*
** Find the appropriate action for a parser given the terminal
** look-ahead token iLookAhead.
**
** If the look-ahead token is YYNOCODE, then check to see if the action is
** independent of the look-ahead.  If it is, return the action, otherwise
** return YY_NO_ACTION.
*/
static int yy_find_shift_action(
  yyParser *pParser,        /* The parser */
  int iLookAhead            /* The look-ahead token */
){
  int i;
  int stateno = pParser->yystack[pParser->yyidx].stateno;

  /* if( pParser->yyidx<0 ) return YY_NO_ACTION;  */
  i = yy_shift_ofst[stateno];
  if( i==YY_SHIFT_USE_DFLT ){
    return yy_default[stateno];
  }
  if( iLookAhead==YYNOCODE ){
    return YY_NO_ACTION;
  }
  i += iLookAhead;
  if( i<0 || i>=YY_SZ_ACTTAB || yy_lookahead[i]!=iLookAhead ){
    return yy_default[stateno];
  }else{
    return yy_action[i];
  }
}

/*
** Find the appropriate action for a parser given the non-terminal
** look-ahead token iLookAhead.
**
** If the look-ahead token is YYNOCODE, then check to see if the action is
** independent of the look-ahead.  If it is, return the action, otherwise
** return YY_NO_ACTION.
*/
static int yy_find_reduce_action(
  yyParser *pParser,        /* The parser */
  int iLookAhead            /* The look-ahead token */
){
  int i;
  int stateno = pParser->yystack[pParser->yyidx].stateno;

  i = yy_reduce_ofst[stateno];
  if( i==YY_REDUCE_USE_DFLT ){
    return yy_default[stateno];
  }
  if( iLookAhead==YYNOCODE ){
    return YY_NO_ACTION;
  }
  i += iLookAhead;
  if( i<0 || i>=YY_SZ_ACTTAB || yy_lookahead[i]!=iLookAhead ){
    return yy_default[stateno];
  }else{
    return yy_action[i];
  }
}

/*
** Perform a shift action.
*/
static void yy_shift(
  yyParser *yypParser,          /* The parser to be shifted */
  int yyNewState,               /* The new state to shift in */
  int yyMajor,                  /* The major token to shift in */
  YYMINORTYPE *yypMinor         /* Pointer ot the minor token to shift in */
){
  yyStackEntry *yytos;
  yypParser->yyidx++;
  if( yypParser->yyidx>=YYSTACKDEPTH ){
     configparserARG_FETCH;
     yypParser->yyidx--;
     while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
     /* Here code is inserted which will execute if the parser
     ** stack every overflows */
     configparserARG_STORE; /* Suppress warning about unused %extra_argument var */
     return;
  }
  yytos = &yypParser->yystack[yypParser->yyidx];
  yytos->stateno = yyNewState;
  yytos->major = yyMajor;
  yytos->minor = *yypMinor;
}

/* The following table contains information about every rule that
** is used during the reduce.
*/
static struct {
  YYCODETYPE lhs;         /* Symbol on the left-hand side of the rule */
  unsigned char nrhs;     /* Number of right-hand side symbols in the rule */
} yyRuleInfo[] = {
  { 27, 1 },
  { 28, 2 },
  { 28, 0 },
  { 29, 1 },
  { 29, 1 },
  { 29, 2 },
  { 29, 1 },
  { 29, 1 },
  { 29, 1 },
  { 30, 3 },
  { 30, 3 },
  { 41, 1 },
  { 36, 3 },
  { 36, 1 },
  { 35, 1 },
  { 35, 1 },
  { 35, 1 },
  { 35, 1 },
  { 40, 2 },
  { 40, 3 },
  { 39, 3 },
  { 39, 2 },
  { 39, 1 },
  { 37, 1 },
  { 37, 3 },
  { 44, 1 },
  { 44, 0 },
  { 45, 1 },
  { 31, 4 },
  { 32, 4 },
  { 32, 1 },
  { 38, 4 },
  { 46, 7 },
  { 43, 1 },
  { 43, 1 },
  { 43, 1 },
  { 43, 1 },
  { 42, 1 },
  { 33, 2 },
  { 34, 2 },
};

static void yy_accept(yyParser*);  /* Forward Declaration */

/*
** Perform a reduce action and the shift that must immediately
** follow the reduce.
*/
static void yy_reduce(
  yyParser *yypParser,         /* The parser */
  int yyruleno                 /* Number of the rule by which to reduce */
){
  int yygoto;                     /* The next state */
  int yyact;                      /* The next action */
  YYMINORTYPE yygotominor;        /* The LHS of the rule reduced */
  yyStackEntry *yymsp;            /* The top of the parser's stack */
  int yysize;                     /* Amount to pop the stack */
  configparserARG_FETCH;
  yymsp = &yypParser->yystack[yypParser->yyidx];

  switch( yyruleno ){
  /* Beginning here are the reduction cases.  A typical example
  ** follows:
  **   case 0:
  **  #line <lineno> <grammarfile>
  **     { ... }           // User supplied code
  **  #line <lineno> <thisfile>
  **     break;
  */
      case 0:
        /* No destructor defined for metalines */
        break;
      case 1:
        /* No destructor defined for metalines */
        /* No destructor defined for metaline */
        break;
      case 2:
        break;
      case 3:
        /* No destructor defined for varline */
        break;
      case 4:
        /* No destructor defined for global */
        break;
      case 5:
{ yymsp[-1].minor.yy78 = NULL; }
  yy_destructor(1,&yymsp[0].minor);
        break;
      case 6:
        /* No destructor defined for include */
        break;
      case 7:
        /* No destructor defined for include_shell */
        break;
      case 8:
  yy_destructor(1,&yymsp[0].minor);
        break;
      case 9:
{
  buffer_copy_string_buffer(yymsp[0].minor.yy41->key, yymsp[-2].minor.yy43);
  if (strncmp(yymsp[-2].minor.yy43->ptr, "env.", sizeof("env.") - 1) == 0) {
    fprintf(stderr, "Setting env variable is not supported in conditional %d %s: %s\n",
        ctx->current->context_ndx,
        ctx->current->key->ptr, yymsp[-2].minor.yy43->ptr);
    ctx->ok = 0;
  } else if (NULL == array_get_element(ctx->current->value, yymsp[0].minor.yy41->key->ptr)) {
    array_insert_unique(ctx->current->value, yymsp[0].minor.yy41);
    yymsp[0].minor.yy41 = NULL;
  } else {
    fprintf(stderr, "Duplicate config variable in conditional %d %s: %s\n",
            ctx->current->context_ndx,
            ctx->current->key->ptr, yymsp[0].minor.yy41->key->ptr);
    ctx->ok = 0;
    yymsp[0].minor.yy41->free(yymsp[0].minor.yy41);
    yymsp[0].minor.yy41 = NULL;
  }
  buffer_free(yymsp[-2].minor.yy43);
  yymsp[-2].minor.yy43 = NULL;
}
  yy_destructor(2,&yymsp[-1].minor);
        break;
      case 10:
{
  array *vars = ctx->current->value;
  data_unset *du;

  if (strncmp(yymsp[-2].minor.yy43->ptr, "env.", sizeof("env.") - 1) == 0) {
    fprintf(stderr, "Appending env variable is not supported in conditional %d %s: %s\n",
        ctx->current->context_ndx,
        ctx->current->key->ptr, yymsp[-2].minor.yy43->ptr);
    ctx->ok = 0;
  } else if (NULL != (du = array_get_element(vars, yymsp[-2].minor.yy43->ptr))) {
    /* exists in current block */
    du = configparser_merge_data(du, yymsp[0].minor.yy41);
    if (NULL == du) {
      ctx->ok = 0;
    }
    else {
      buffer_copy_string_buffer(du->key, yymsp[-2].minor.yy43);
      array_replace(vars, du);
    }
    yymsp[0].minor.yy41->free(yymsp[0].minor.yy41);
  } else if (NULL != (du = configparser_get_variable(ctx, yymsp[-2].minor.yy43))) {
    du = configparser_merge_data(du, yymsp[0].minor.yy41);
    if (NULL == du) {
      ctx->ok = 0;
    }
    else {
      buffer_copy_string_buffer(du->key, yymsp[-2].minor.yy43);
      array_insert_unique(ctx->current->value, du);
    }
    yymsp[0].minor.yy41->free(yymsp[0].minor.yy41);
  } else {
    buffer_copy_string_buffer(yymsp[0].minor.yy41->key, yymsp[-2].minor.yy43);
    array_insert_unique(ctx->current->value, yymsp[0].minor.yy41);
  }
  buffer_free(yymsp[-2].minor.yy43);
  yymsp[-2].minor.yy43 = NULL;
  yymsp[0].minor.yy41 = NULL;
}
  yy_destructor(3,&yymsp[-1].minor);
        break;
      case 11:
{
  if (strchr(yymsp[0].minor.yy0->ptr, '.') == NULL) {
    yygotominor.yy43 = buffer_init_string("var.");
    buffer_append_string_buffer(yygotominor.yy43, yymsp[0].minor.yy0);
    buffer_free(yymsp[0].minor.yy0);
    yymsp[0].minor.yy0 = NULL;
  } else {
    yygotominor.yy43 = yymsp[0].minor.yy0;
    yymsp[0].minor.yy0 = NULL;
  }
}
        break;
      case 12:
{
  yygotominor.yy41 = configparser_merge_data(yymsp[-2].minor.yy41, yymsp[0].minor.yy41);
  if (NULL == yygotominor.yy41) {
    ctx->ok = 0;
  }
  yymsp[-2].minor.yy41 = NULL;
  yymsp[0].minor.yy41->free(yymsp[0].minor.yy41);
  yymsp[0].minor.yy41 = NULL;
}
  yy_destructor(5,&yymsp[-1].minor);
        break;
      case 13:
{
  yygotominor.yy41 = yymsp[0].minor.yy41;
  yymsp[0].minor.yy41 = NULL;
}
        break;
      case 14:
{
  yygotominor.yy41 = NULL;
  if (strncmp(yymsp[0].minor.yy43->ptr, "env.", sizeof("env.") - 1) == 0) {
    char *env;

    if (NULL != (env = getenv(yymsp[0].minor.yy43->ptr + 4))) {
      data_string *ds;
      ds = data_string_init();
      buffer_append_string(ds->value, env);
      yygotominor.yy41 = (data_unset *)ds;
    }
    else {
      fprintf(stderr, "Undefined env variable: %s\n", yymsp[0].minor.yy43->ptr + 4);
      ctx->ok = 0;
    }
  } else if (NULL == (yygotominor.yy41 = configparser_get_variable(ctx, yymsp[0].minor.yy43))) {
    fprintf(stderr, "Undefined config variable: %s\n", yymsp[0].minor.yy43->ptr);
    ctx->ok = 0;
  }
  if (!yygotominor.yy41) {
    /* make a dummy so it won't crash */
    yygotominor.yy41 = (data_unset *)data_string_init();
  }
  buffer_free(yymsp[0].minor.yy43);
  yymsp[0].minor.yy43 = NULL;
}
        break;
      case 15:
{
  yygotominor.yy41 = (data_unset *)data_string_init();
  buffer_copy_string_buffer(((data_string *)(yygotominor.yy41))->value, yymsp[0].minor.yy0);
  buffer_free(yymsp[0].minor.yy0);
  yymsp[0].minor.yy0 = NULL;
}
        break;
      case 16:
{
  yygotominor.yy41 = (data_unset *)data_integer_init();
  ((data_integer *)(yygotominor.yy41))->value = strtol(yymsp[0].minor.yy0->ptr, NULL, 10);
  buffer_free(yymsp[0].minor.yy0);
  yymsp[0].minor.yy0 = NULL;
}
        break;
      case 17:
{
  yygotominor.yy41 = (data_unset *)data_array_init();
  array_free(((data_array *)(yygotominor.yy41))->value);
  ((data_array *)(yygotominor.yy41))->value = yymsp[0].minor.yy40;
  yymsp[0].minor.yy40 = NULL;
}
        break;
      case 18:
{
  yygotominor.yy40 = array_init();
}
  yy_destructor(8,&yymsp[-1].minor);
  yy_destructor(9,&yymsp[0].minor);
        break;
      case 19:
{
  yygotominor.yy40 = yymsp[-1].minor.yy40;
  yymsp[-1].minor.yy40 = NULL;
}
  yy_destructor(8,&yymsp[-2].minor);
  yy_destructor(9,&yymsp[0].minor);
        break;
      case 20:
{
  if (buffer_is_empty(yymsp[0].minor.yy41->key) ||
      NULL == array_get_element(yymsp[-2].minor.yy40, yymsp[0].minor.yy41->key->ptr)) {
    array_insert_unique(yymsp[-2].minor.yy40, yymsp[0].minor.yy41);
    yymsp[0].minor.yy41 = NULL;
  } else {
    fprintf(stderr, "Duplicate array-key: %s\n",
            yymsp[0].minor.yy41->key->ptr);
    ctx->ok = 0;
    yymsp[0].minor.yy41->free(yymsp[0].minor.yy41);
    yymsp[0].minor.yy41 = NULL;
  }

  yygotominor.yy40 = yymsp[-2].minor.yy40;
  yymsp[-2].minor.yy40 = NULL;
}
  yy_destructor(10,&yymsp[-1].minor);
        break;
      case 21:
{
  yygotominor.yy40 = yymsp[-1].minor.yy40;
  yymsp[-1].minor.yy40 = NULL;
}
  yy_destructor(10,&yymsp[0].minor);
        break;
      case 22:
{
  yygotominor.yy40 = array_init();
  array_insert_unique(yygotominor.yy40, yymsp[0].minor.yy41);
  yymsp[0].minor.yy41 = NULL;
}
        break;
      case 23:
{
  yygotominor.yy41 = yymsp[0].minor.yy41;
  yymsp[0].minor.yy41 = NULL;
}
        break;
      case 24:
{
  buffer_copy_string_buffer(yymsp[0].minor.yy41->key, yymsp[-2].minor.yy43);
  buffer_free(yymsp[-2].minor.yy43);
  yymsp[-2].minor.yy43 = NULL;

  yygotominor.yy41 = yymsp[0].minor.yy41;
  yymsp[0].minor.yy41 = NULL;
}
  yy_destructor(11,&yymsp[-1].minor);
        break;
      case 25:
  yy_destructor(1,&yymsp[0].minor);
        break;
      case 26:
        break;
      case 27:
{
  data_config *dc;
  dc = (data_config *)array_get_element(ctx->srv->config_context, "global");
  assert(dc);
  configparser_push(ctx, dc, 0);
}
  yy_destructor(12,&yymsp[0].minor);
        break;
      case 28:
{
  data_config *cur;

  cur = ctx->current;
  configparser_pop(ctx);

  assert(cur && ctx->current);

  yygotominor.yy78 = cur;
}
        /* No destructor defined for globalstart */
  yy_destructor(13,&yymsp[-2].minor);
        /* No destructor defined for metalines */
  yy_destructor(14,&yymsp[0].minor);
        break;
      case 29:
{
  assert(yymsp[-3].minor.yy78->context_ndx < yymsp[0].minor.yy78->context_ndx);
  yymsp[0].minor.yy78->prev = yymsp[-3].minor.yy78;
  yymsp[-3].minor.yy78->next = yymsp[0].minor.yy78;
  yygotominor.yy78 = yymsp[0].minor.yy78;
  yymsp[-3].minor.yy78 = NULL;
  yymsp[0].minor.yy78 = NULL;
}
        /* No destructor defined for eols */
  yy_destructor(15,&yymsp[-1].minor);
        break;
      case 30:
{
  yygotominor.yy78 = yymsp[0].minor.yy78;
  yymsp[0].minor.yy78 = NULL;
}
        break;
      case 31:
{
  data_config *cur;

  cur = ctx->current;
  configparser_pop(ctx);

  assert(cur && ctx->current);

  yygotominor.yy78 = cur;
}
        /* No destructor defined for context */
  yy_destructor(13,&yymsp[-2].minor);
        /* No destructor defined for metalines */
  yy_destructor(14,&yymsp[0].minor);
        break;
      case 32:
{
  data_config *dc;
  conf_buffer *b, *rvalue, *op;

  if (ctx->ok && yymsp[0].minor.yy41->type != TYPE_STRING) {
    fprintf(stderr, "rvalue must be string");
    ctx->ok = 0;
  }

  switch(yymsp[-1].minor.yy27) {
  case CONFIG_COND_NE:
    op = buffer_init_string("!=");
    break;
  case CONFIG_COND_EQ:
    op = buffer_init_string("==");
    break;
  default:
    assert(0);
    return;
  }

  b = buffer_init();
  buffer_copy_string_buffer(b, ctx->current->key);
  buffer_append_string(b, "/");
  buffer_append_string_buffer(b, yymsp[-5].minor.yy0);
  buffer_append_string_buffer(b, yymsp[-3].minor.yy43);
  buffer_append_string_buffer(b, op);
  rvalue = ((data_string*)yymsp[0].minor.yy41)->value;
  buffer_append_string_buffer(b, rvalue);

  if (NULL != (dc = (data_config *)array_get_element(ctx->all_configs, b->ptr))) {
    configparser_push(ctx, dc, 0);
  } else {
    static const struct {
      comp_key_t comp;
      char comp_key[20];
      size_t len;
    } comps[] = {
      { COMP_SERVER_SOCKET,      CONST_STR_LEN("SERVER[\"socket\"]"   ) },
      { COMP_HTTP_URL,           CONST_STR_LEN("HTTP[\"url\"]"        ) },
      { COMP_HTTP_HOST,          CONST_STR_LEN("HTTP[\"host\"]"       ) },
      { COMP_HTTP_REFERER,       CONST_STR_LEN("HTTP[\"referer\"]"    ) },
      { COMP_HTTP_USERAGENT,     CONST_STR_LEN("HTTP[\"useragent\"]"  ) },
      { COMP_HTTP_COOKIE,        CONST_STR_LEN("HTTP[\"cookie\"]"     ) },
      { COMP_HTTP_REMOTEIP,      CONST_STR_LEN("HTTP[\"remoteip\"]"   ) },
      { COMP_HTTP_QUERYSTRING,   CONST_STR_LEN("HTTP[\"querystring\"]") },
      { COMP_UNSET, "", 0 },
    };
    size_t i;

    dc = data_config_init();

    buffer_copy_string_buffer(dc->key, b);
    buffer_copy_string_buffer(dc->op, op);
    buffer_copy_string_buffer(dc->comp_key, yymsp[-5].minor.yy0);
    buffer_append_string_len(dc->comp_key, CONST_STR_LEN("[\""));
    buffer_append_string_buffer(dc->comp_key, yymsp[-3].minor.yy43);
    buffer_append_string_len(dc->comp_key, CONST_STR_LEN("\"]"));
    dc->cond = yymsp[-1].minor.yy27;

    for (i = 0; comps[i].len; i ++) {
      if (buffer_is_equal_string(
            dc->comp_key, comps[i].comp_key, comps[i].len)) {
        dc->comp = comps[i].comp;
        break;
      }
    }
    if (COMP_UNSET == dc->comp) {
      fprintf(stderr, "error comp_key %s", dc->comp_key->ptr);
      ctx->ok = 0;
    }

    switch(yymsp[-1].minor.yy27) {
    case CONFIG_COND_NE:
    case CONFIG_COND_EQ:
      dc->string = buffer_init_buffer(rvalue);
      break;

    default:
      fprintf(stderr, "unknown condition for $%s[%s]\n",
                      yymsp[-5].minor.yy0->ptr, yymsp[-3].minor.yy43->ptr);
      ctx->ok = 0;
      break;
    }

    configparser_push(ctx, dc, 1);
  }

  buffer_free(b);
  buffer_free(op);
  buffer_free(yymsp[-5].minor.yy0);
  yymsp[-5].minor.yy0 = NULL;
  buffer_free(yymsp[-3].minor.yy43);
  yymsp[-3].minor.yy43 = NULL;
  yymsp[0].minor.yy41->free(yymsp[0].minor.yy41);
  yymsp[0].minor.yy41 = NULL;
}
  yy_destructor(16,&yymsp[-6].minor);
  yy_destructor(18,&yymsp[-4].minor);
  yy_destructor(19,&yymsp[-2].minor);
        break;
      case 33:
{
  yygotominor.yy27 = CONFIG_COND_EQ;
}
  yy_destructor(20,&yymsp[0].minor);
        break;
      case 35:
{
  yygotominor.yy27 = CONFIG_COND_NE;
}
  yy_destructor(22,&yymsp[0].minor);
        break;
      case 37:
{
  yygotominor.yy43 = NULL;
  if (ctx->ok) {
    if (yymsp[0].minor.yy41->type == TYPE_STRING) {
      yygotominor.yy43 = buffer_init_buffer(((data_string*)yymsp[0].minor.yy41)->value);
    } else if (yymsp[0].minor.yy41->type == TYPE_INTEGER) {
      yygotominor.yy43 = buffer_init();
      buffer_copy_long(yygotominor.yy43, ((data_integer *)yymsp[0].minor.yy41)->value);
    } else {
      fprintf(stderr, "operand must be string");
      ctx->ok = 0;
    }
  }
  yymsp[0].minor.yy41->free(yymsp[0].minor.yy41);
  yymsp[0].minor.yy41 = NULL;
}
        break;
      case 38:
{
  if (ctx->ok) {
    if (0 != config_parse_file(ctx, yymsp[0].minor.yy43->ptr)) {
      ctx->ok = 0;
    }
    buffer_free(yymsp[0].minor.yy43);
    yymsp[0].minor.yy43 = NULL;
  }
}
  yy_destructor(24,&yymsp[-1].minor);
        break;
      case 39:
{
  if (ctx->ok) {
    if (0 != config_parse_cmd(ctx, yymsp[0].minor.yy43->ptr)) {
      ctx->ok = 0;
    }
    buffer_free(yymsp[0].minor.yy43);
    yymsp[0].minor.yy43 = NULL;
  }
}
  yy_destructor(25,&yymsp[-1].minor);
        break;
  };
  yygoto = yyRuleInfo[yyruleno].lhs;
  yysize = yyRuleInfo[yyruleno].nrhs;
  yypParser->yyidx -= yysize;
  yyact = yy_find_reduce_action(yypParser,yygoto);
  if( yyact < YYNSTATE ){
    yy_shift(yypParser,yyact,yygoto,&yygotominor);
  }else if( yyact == YYNSTATE + YYNRULE + 1 ){
    yy_accept(yypParser);
  }
}

/*
** The following code executes when the parse fails
*/
static void yy_parse_failed(
  yyParser *yypParser           /* The parser */
){
  configparserARG_FETCH;
  while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser fails */

  ctx->ok = 0;

  configparserARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/*
** The following code executes when a syntax error first occurs.
*/
static void yy_syntax_error(
  yyParser *yypParser           /* The parser */
){
  configparserARG_FETCH;
  configparserARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/*
** The following is executed when the parser accepts
*/
static void yy_accept(
  yyParser *yypParser           /* The parser */
){
  configparserARG_FETCH;
  while( yypParser->yyidx>=0 ) yy_pop_parser_stack(yypParser);
  /* Here code is inserted which will be executed whenever the
  ** parser accepts */
  configparserARG_STORE; /* Suppress warning about unused %extra_argument variable */
}

/* The main parser program.
** The first argument is a pointer to a structure obtained from
** "configparserAlloc" which describes the current state of the parser.
** The second argument is the major token number.  The third is
** the minor token.  The fourth optional argument is whatever the
** user wants (and specified in the grammar) and is available for
** use by the action routines.
**
** Inputs:
** <ul>
** <li> A pointer to the parser (an opaque structure.)
** <li> The major token number.
** <li> The minor token number.
** <li> An option argument of a grammar-specified type.
** </ul>
**
** Outputs:
** None.
*/
void configparser(
  void *yyp,                   /* The parser */
  int yymajor,                 /* The major token code number */
  configparserTOKENTYPE yyminor       /* The value for the token */
  configparserARG_PDECL               /* Optional %extra_argument parameter */
){
  YYMINORTYPE yyminorunion;
  int yyact;            /* The parser action. */
  int yyendofinput;     /* True if we are at the end of input */
  int yyerrorhit = 0;   /* True if yymajor has invoked an error */
  yyParser *yypParser;  /* The parser */

  /* (re)initialize the parser, if necessary */
  yypParser = (yyParser*)yyp;
  if( yypParser->yyidx<0 ){
    if( yymajor==0 ) return;
    yypParser->yyidx = 0;
    yypParser->yyerrcnt = -1;
    yypParser->yystack[0].stateno = 0;
    yypParser->yystack[0].major = 0;
  }
  yyminorunion.yy0 = yyminor;
  yyendofinput = (yymajor==0);
  configparserARG_STORE;

  do{
    yyact = yy_find_shift_action(yypParser,yymajor);
    if( yyact<YYNSTATE ){
      yy_shift(yypParser,yyact,yymajor,&yyminorunion);
      yypParser->yyerrcnt--;
      if( yyendofinput && yypParser->yyidx>=0 ){
        yymajor = 0;
      }else{
        yymajor = YYNOCODE;
      }
    }else if( yyact < YYNSTATE + YYNRULE ){
      yy_reduce(yypParser,yyact-YYNSTATE);
    }else if( yyact == YY_ERROR_ACTION ){
      int yymx;
#ifdef YYERRORSYMBOL
      /* A syntax error has occurred.
      ** The response to an error depends upon whether or not the
      ** grammar defines an error token "ERROR".
      **
      ** This is what we do if the grammar does define ERROR:
      **
      **  * Call the %syntax_error function.
      **
      **  * Begin popping the stack until we enter a state where
      **    it is legal to shift the error symbol, then shift
      **    the error symbol.
      **
      **  * Set the error count to three.
      **
      **  * Begin accepting and shifting new tokens.  No new error
      **    processing will occur until three tokens have been
      **    shifted successfully.
      **
      */
      if( yypParser->yyerrcnt<0 ){
        yy_syntax_error(yypParser);
      }
      yymx = yypParser->yystack[yypParser->yyidx].major;
      if( yymx==YYERRORSYMBOL || yyerrorhit ){
        yy_destructor(yymajor,&yyminorunion);
        yymajor = YYNOCODE;
      }else{
         while(
          yypParser->yyidx >= 0 &&
          yymx != YYERRORSYMBOL &&
          (yyact = yy_find_shift_action(yypParser,YYERRORSYMBOL)) >= YYNSTATE
        ){
          yy_pop_parser_stack(yypParser);
        }
        if( yypParser->yyidx < 0 || yymajor==0 ){
          yy_destructor(yymajor,&yyminorunion);
          yy_parse_failed(yypParser);
          yymajor = YYNOCODE;
        }else if( yymx!=YYERRORSYMBOL ){
          YYMINORTYPE u2;
          u2.YYERRSYMDT = 0;
          yy_shift(yypParser,yyact,YYERRORSYMBOL,&u2);
        }
      }
      yypParser->yyerrcnt = 3;
      yyerrorhit = 1;
#else  /* YYERRORSYMBOL is not defined */
      /* This is what we do if the grammar does not define ERROR:
      **
      **  * Report an error message, and throw away the input token.
      **
      **  * If the input token is $, then fail the parse.
      **
      ** As before, subsequent error messages are suppressed until
      ** three input tokens have been successfully shifted.
      */
      if( yypParser->yyerrcnt<=0 ){
        yy_syntax_error(yypParser);
      }
      yypParser->yyerrcnt = 3;
      yy_destructor(yymajor,&yyminorunion);
      if( yyendofinput ){
        yy_parse_failed(yypParser);
      }
      yymajor = YYNOCODE;
#endif
    }else{
      yy_accept(yypParser);
      yymajor = YYNOCODE;
    }
  }while( yymajor!=YYNOCODE && yypParser->yyidx>=0 );
  return;
}
