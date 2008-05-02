/* Copyright (c) 2007 by Ian Piumarta
 * All rights reserved.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the 'Software'),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, provided that the above copyright notice(s) and this
 * permission notice appear in all copies of the Software.  Acknowledgement
 * of the use of this Software in supporting documentation would be
 * appreciated but is not required.
 * 
 * THE SOFTWARE IS PROVIDED 'AS IS'.  USE ENTIRELY AT YOUR OWN RISK.
 * 
 * Last edited: 2007-08-31 13:55:23 by piumarta on emilia.local
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "version.h"
#include "tree.h"

static int yyl(void)
{
  static int prev= 0;
  return ++prev;
}

static void charClassSet  (unsigned char bits[], int c)	{ bits[c >> 3] |=  (1 << (c & 7)); }
static void charClassClear(unsigned char bits[], int c)	{ bits[c >> 3] &= ~(1 << (c & 7)); }

typedef void (*setter)(unsigned char bits[], int c);

static char *makeCharClass(unsigned char *cclass)
{
  unsigned char	 bits[32];
  setter	 set;
  int		 c, prev= -1;
  static char	 string[256];
  char		*ptr;

  if ('^' == *cclass)
    {
      memset(bits, 255, 32);
      set= charClassClear;
      ++cclass;
    }
  else
    {
      memset(bits, 0, 32);
      set= charClassSet;
    }
  while ((c= *cclass++))
    {
      if ('-' == c && *cclass && prev >= 0)
	{
	  for (c= *cclass++;  prev <= c;  ++prev)
	    set(bits, prev);
	  prev= -1;
	}
      else if ('\\' == c && *cclass)
	{
	  switch (c= *cclass++)
	    {
	    case 'a':  c= '\a'; break;	/* bel */
	    case 'b':  c= '\b'; break;	/* bs */
	    case 'e':  c= '\e'; break;	/* esc */
	    case 'f':  c= '\f'; break;	/* ff */
	    case 'n':  c= '\n'; break;	/* nl */
	    case 'r':  c= '\r'; break;	/* cr */
	    case 't':  c= '\t'; break;	/* ht */
	    case 'v':  c= '\v'; break;	/* vt */
	    default:		break;
	    }
	  set(bits, prev= c);
	}
      else
	set(bits, prev= c);
    }

  ptr= string;
  for (c= 0;  c < 32;  ++c)
    ptr += sprintf(ptr, "\\%03o", bits[c]);

  return string;
}

static void begin(void)		{ fprintf(output, "\n  {"); }
static void end(void)		{ fprintf(output, "\n  }"); }
static void label(int n)	{ fprintf(output, "\n  l%d:;\t", n); }
static void jump(int n)		{ fprintf(output, "  goto l%d;", n); }
static void save(int n)		{ fprintf(output, "  int yypos%d= yypos, yythunkpos%d= yythunkpos;", n, n); }
static void restore(int n)	{ fprintf(output,     "  yypos= yypos%d; yythunkpos= yythunkpos%d;", n, n); }

static void Node_compile_c_ko(Node *node, int ko)
{
  assert(node);
  switch (node->type)
    {
    case Rule:
      fprintf(stderr, "\ninternal error #1 (%s)\n", node->rule.name);
      exit(1);
      break;

    case Dot:
      fprintf(output, "  if (!yymatchDot()) goto l%d;", ko);
      break;

    case Name:
      fprintf(output, "  if (!yy_%s()) goto l%d;", node->name.rule->rule.name, ko);
      if (node->name.variable)
	fprintf(output, "  yyDo(yySet, %d, 0);", node->name.variable->variable.offset);
      break;

    case Character:
    case String:
      {
	int len= strlen(node->string.value);
	if (1 == len || (2 == len && '\\' == node->string.value[0]))
	  fprintf(output, "  if (!yymatchChar('%s')) goto l%d;", node->string.value, ko);
	else
	  fprintf(output, "  if (!yymatchString(\"%s\")) goto l%d;", node->string.value, ko);
      }
      break;

    case Class:
      fprintf(output, "  if (!yymatchClass((unsigned char *)\"%s\")) goto l%d;", makeCharClass(node->cclass.value), ko);
      break;

    case Action:
      fprintf(output, "  yyDo(yy%s, yybegin, yyend);", node->action.name);
      break;

    case Predicate:
      fprintf(output, "  yyText(yybegin, yyend);  if (!(%s)) goto l%d;", node->action.text, ko);
      break;

    case Alternate:
      {
	int ok= yyl();
	begin();
	save(ok);
	for (node= node->alternate.first;  node;  node= node->alternate.next)
	  if (node->alternate.next)
	    {
	      int next= yyl();
	      Node_compile_c_ko(node, next);
	      jump(ok);
	      label(next);
	      restore(ok);
	    }
	  else
	    Node_compile_c_ko(node, ko);
	end();
	label(ok);
      }
      break;

    case Sequence:
      for (node= node->sequence.first;  node;  node= node->sequence.next)
	Node_compile_c_ko(node, ko);
      break;

    case PeekFor:
      {
	int ok= yyl();
	begin();
	save(ok);
	Node_compile_c_ko(node->peekFor.element, ko);
	restore(ok);
	end();
      }
      break;

    case PeekNot:
      {
	int ok= yyl();
	begin();
	save(ok);
	Node_compile_c_ko(node->peekFor.element, ok);
	jump(ko);
	label(ok);
	restore(ok);
	end();
      }
      break;

    case Query:
      {
	int qko= yyl(), qok= yyl();
	begin();
	save(qko);
	Node_compile_c_ko(node->query.element, qko);
	jump(qok);
	label(qko);
	restore(qko);
	end();
	label(qok);
      }
      break;

    case Star:
      {
	int again= yyl(), out= yyl();
	label(again);
	begin();
	save(out);
	Node_compile_c_ko(node->star.element, out);
	jump(again);
	label(out);
	restore(out);
	end();
      }
      break;

    case Plus:
      {
	int again= yyl(), out= yyl();
	Node_compile_c_ko(node->plus.element, ko);
	label(again);
	begin();
	save(out);
	Node_compile_c_ko(node->plus.element, out);
	jump(again);
	label(out);
	restore(out);
	end();
      }
      break;

    default:
      fprintf(stderr, "\nNode_compile_c_ko: illegal node type %d\n", node->type);
      exit(1);
    }
}


static int countVariables(Node *node)
{
  int count= 0;
  while (node)
    {
      ++count;
      node= node->variable.next;
    }
  return count;
}

static void defineVariables(Node *node)
{
  int count= 0;
  while (node)
    {
      fprintf(output, "#define %s yyval[%d]\n", node->variable.name, --count);
      node->variable.offset= count;
      node= node->variable.next;
    }
}

static void undefineVariables(Node *node)
{
  while (node)
    {
      fprintf(output, "#undef %s\n", node->variable.name);
      node= node->variable.next;
    }
}


static void Rule_compile_c2(Node *node)
{
  assert(node);
  assert(Rule == node->type);

  if (!node->rule.expression)
    fprintf(stderr, "rule '%s' used but not defined\n", node->rule.name);
  else
    {
      int ko= yyl(), safe;

      if ((!(RuleUsed & node->rule.flags)) && (node != start))
	fprintf(stderr, "rule '%s' defined but not used\n", node->rule.name);

      safe= ((Query == node->rule.expression->type) || (Star == node->rule.expression->type));

      fprintf(output, "\nYY_RULE(int) yy_%s()\n{", node->rule.name);
      if (!safe) save(0);
      if (node->rule.variables)
	fprintf(output, "  yyDo(yyPush, %d, 0);", countVariables(node->rule.variables));
      fprintf(output, "\n  yyprintf((stderr, \"%%s\\n\", \"%s\"));", node->rule.name);
      Node_compile_c_ko(node->rule.expression, ko);
      fprintf(output, "\n  yyprintf((stderr, \"  ok   %%s @ %%s\\n\", \"%s\", yybuf+yypos));", node->rule.name);
      if (node->rule.variables)
	fprintf(output, "  yyDo(yyPop, %d, 0);", countVariables(node->rule.variables));
      fprintf(output, "\n  return 1;");
      if (!safe)
	{
	  label(ko);
	  restore(0);
	  fprintf(output, "\n  yyprintf((stderr, \"  fail %%s @ %%s\\n\", \"%s\", yybuf+yypos));", node->rule.name);
	  fprintf(output, "\n  return 0;");
	}
      fprintf(output, "\n}");
    }

  if (node->rule.next)
    Rule_compile_c2(node->rule.next);
}

static char *header= "\
#include <stdio.h>\n\
#include <stdlib.h>\n\
#include <string.h>\n\
";

static char *preamble= "\
#ifndef YY_VARIABLE\n\
#define YY_VARIABLE(T)	static T\n\
#endif\n\
#ifndef YY_LOCAL\n\
#define YY_LOCAL(T)	static T\n\
#endif\n\
#ifndef YY_ACTION\n\
#define YY_ACTION(T)	static T\n\
#endif\n\
#ifndef YY_RULE\n\
#define YY_RULE(T)	static T\n\
#endif\n\
#ifndef YY_PARSE\n\
#define YY_PARSE(T)	T\n\
#endif\n\
#ifndef YYPARSE\n\
#define YYPARSE		yyparse\n\
#endif\n\
#ifndef YYPARSEFROM\n\
#define YYPARSEFROM	yyparsefrom\n\
#endif\n\
#ifndef YY_INPUT\n\
#define YY_INPUT(buf, result, max_size)			\\\n\
  {							\\\n\
    int yyc= getchar();					\\\n\
    result= (EOF == yyc) ? 0 : (*(buf)= yyc, 1);	\\\n\
    yyprintf((stderr, \"<%c>\", yyc));			\\\n\
  }\n\
#endif\n\
#ifndef YY_BEGIN\n\
#define YY_BEGIN	( yybegin= yypos, 1)\n\
#endif\n\
#ifndef YY_END\n\
#define YY_END		( yyend= yypos, 1)\n\
#endif\n\
#ifdef YY_DEBUG\n\
# define yyprintf(args)	fprintf args\n\
#else\n\
# define yyprintf(args)\n\
#endif\n\
#ifndef YYSTYPE\n\
#define YYSTYPE	int\n\
#endif\n\
\n\
#ifndef YY_PART\n\
\n\
typedef void (*yyaction)(char *yytext, int yyleng);\n\
typedef struct _yythunk { int begin, end;  yyaction  action;  struct _yythunk *next; } yythunk;\n\
\n\
YY_VARIABLE(char *   ) yybuf= 0;\n\
YY_VARIABLE(int	     ) yybuflen= 0;\n\
YY_VARIABLE(int	     ) yypos= 0;\n\
YY_VARIABLE(int	     ) yylimit= 0;\n\
YY_VARIABLE(char *   ) yytext= 0;\n\
YY_VARIABLE(int	     ) yytextlen= 0;\n\
YY_VARIABLE(int	     ) yybegin= 0;\n\
YY_VARIABLE(int	     ) yyend= 0;\n\
YY_VARIABLE(int	     ) yytextmax= 0;\n\
YY_VARIABLE(yythunk *) yythunks= 0;\n\
YY_VARIABLE(int	     ) yythunkslen= 0;\n\
YY_VARIABLE(int      ) yythunkpos= 0;\n\
YY_VARIABLE(YYSTYPE  ) yy;\n\
YY_VARIABLE(YYSTYPE *) yyval= 0;\n\
YY_VARIABLE(YYSTYPE *) yyvals= 0;\n\
YY_VARIABLE(int      ) yyvalslen= 0;\n\
\n\
YY_LOCAL(int) yyrefill(void)\n\
{\n\
  int yyn;\n\
  while (yybuflen - yypos < 512)\n\
    {\n\
      yybuflen *= 2;\n\
      yybuf= realloc(yybuf, yybuflen);\n\
    }\n\
  YY_INPUT((yybuf + yypos), yyn, (yybuflen - yypos));\n\
  if (!yyn) return 0;\n\
  yylimit += yyn;\n\
  return 1;\n\
}\n\
\n\
YY_LOCAL(int) yymatchDot(void)\n\
{\n\
  if (yypos >= yylimit && !yyrefill()) return 0;\n\
  ++yypos;\n\
  return 1;\n\
}\n\
\n\
YY_LOCAL(int) yymatchChar(int c)\n\
{\n\
  if (yypos >= yylimit && !yyrefill()) return 0;\n\
  if (yybuf[yypos] == c)\n\
    {\n\
      ++yypos;\n\
      yyprintf((stderr, \"  ok   yymatchChar(%c) @ %s\\n\", c, yybuf+yypos));\n\
      return 1;\n\
    }\n\
  yyprintf((stderr, \"  fail yymatchChar(%c) @ %s\\n\", c, yybuf+yypos));\n\
  return 0;\n\
}\n\
\n\
YY_LOCAL(int) yymatchString(char *s)\n\
{\n\
  int yysav= yypos;\n\
  while (*s)\n\
    {\n\
      if (yypos >= yylimit && !yyrefill()) return 0;\n\
      if (yybuf[yypos] != *s)\n\
        {\n\
          yypos= yysav;\n\
          return 0;\n\
        }\n\
      ++s;\n\
      ++yypos;\n\
    }\n\
  return 1;\n\
}\n\
\n\
YY_LOCAL(int) yymatchClass(unsigned char *bits)\n\
{\n\
  int c;\n\
  if (yypos >= yylimit && !yyrefill()) return 0;\n\
  c= yybuf[yypos];\n\
  if (bits[c >> 3] & (1 << (c & 7)))\n\
    {\n\
      ++yypos;\n\
      yyprintf((stderr, \"  ok   yymatchClass @ %s\\n\", yybuf+yypos));\n\
      return 1;\n\
    }\n\
  yyprintf((stderr, \"  fail yymatchClass @ %s\\n\", yybuf+yypos));\n\
  return 0;\n\
}\n\
\n\
YY_LOCAL(void) yyDo(yyaction action, int begin, int end)\n\
{\n\
  while (yythunkpos >= yythunkslen)\n\
    {\n\
      yythunkslen *= 2;\n\
      yythunks= realloc(yythunks, sizeof(yythunk) * yythunkslen);\n\
    }\n\
  yythunks[yythunkpos].begin=  begin;\n\
  yythunks[yythunkpos].end=    end;\n\
  yythunks[yythunkpos].action= action;\n\
  ++yythunkpos;\n\
}\n\
\n\
YY_LOCAL(int) yyText(int begin, int end)\n\
{\n\
  int yyleng= end - begin;\n\
  if (yyleng <= 0)\n\
    yyleng= 0;\n\
  else\n\
    {\n\
      while (yytextlen < (yyleng - 1))\n\
	{\n\
	  yytextlen *= 2;\n\
	  yytext= realloc(yytext, yytextlen);\n\
	}\n\
      memcpy(yytext, yybuf + begin, yyleng);\n\
    }\n\
  yytext[yyleng]= '\\0';\n\
  return yyleng;\n\
}\n\
\n\
YY_LOCAL(void) yyDone(void)\n\
{\n\
  int pos;\n\
  for (pos= 0;  pos < yythunkpos;  ++pos)\n\
    {\n\
      yythunk *thunk= &yythunks[pos];\n\
      int yyleng= thunk->end ? yyText(thunk->begin, thunk->end) : thunk->begin;\n\
      yyprintf((stderr, \"DO [%d] %p %s\\n\", pos, thunk->action, yytext));\n\
      thunk->action(yytext, yyleng);\n\
    }\n\
  yythunkpos= 0;\n\
}\n\
\n\
YY_LOCAL(void) yyCommit()\n\
{\n\
  if ((yylimit -= yypos))\n\
    {\n\
      memmove(yybuf, yybuf + yypos, yylimit);\n\
    }\n\
  yybegin -= yypos;\n\
  yyend -= yypos;\n\
  yypos= yythunkpos= 0;\n\
}\n\
\n\
YY_LOCAL(int) yyAccept(int tp0)\n\
{\n\
  if (tp0)\n\
    {\n\
      fprintf(stderr, \"accept denied at %d\\n\", tp0);\n\
      return 0;\n\
    }\n\
  else\n\
    {\n\
      yyDone();\n\
      yyCommit();\n\
    }\n\
  return 1;\n\
}\n\
\n\
YY_LOCAL(void) yyPush(char *text, int count)	{ yyval += count; }\n\
YY_LOCAL(void) yyPop(char *text, int count)	{ yyval -= count; }\n\
YY_LOCAL(void) yySet(char *text, int count)	{ yyval[count]= yy; }\n\
\n\
#endif /* YY_PART */\n\
\n\
#define	YYACCEPT	yyAccept(yythunkpos0)\n\
\n\
";

static char *footer= "\n\
\n\
#ifndef YY_PART\n\
\n\
typedef int (*yyrule)();\n\
\n\
YY_PARSE(int) YYPARSEFROM(yyrule yystart)\n\
{\n\
  int yyok;\n\
  if (!yybuflen)\n\
    {\n\
      yybuflen= 1024;\n\
      yybuf= malloc(yybuflen);\n\
      yytextlen= 1024;\n\
      yytext= malloc(yytextlen);\n\
      yythunkslen= 32;\n\
      yythunks= malloc(sizeof(yythunk) * yythunkslen);\n\
      yyvalslen= 32;\n\
      yyvals= malloc(sizeof(YYSTYPE) * yyvalslen);\n\
      yybegin= yyend= yypos= yylimit= yythunkpos= 0;\n\
    }\n\
  yybegin= yyend= yypos;\n\
  yythunkpos= 0;\n\
  yyval= yyvals;\n\
  yyok= yystart();\n\
  if (yyok) yyDone();\n\
  yyCommit();\n\
  return yyok;\n\
  (void)yyrefill;\n\
  (void)yymatchDot;\n\
  (void)yymatchChar;\n\
  (void)yymatchString;\n\
  (void)yymatchClass;\n\
  (void)yyDo;\n\
  (void)yyText;\n\
  (void)yyDone;\n\
  (void)yyCommit;\n\
  (void)yyAccept;\n\
  (void)yyPush;\n\
  (void)yyPop;\n\
  (void)yySet;\n\
  (void)yytextmax;\n\
}\n\
\n\
YY_PARSE(int) YYPARSE(void)\n\
{\n\
  return YYPARSEFROM(yy_%s);\n\
}\n\
\n\
#endif\n\
";

void Rule_compile_c_header(void)
{
  fprintf(output, "/* A recursive-descent parser generated by peg %d.%d.%d */\n", PEG_MAJOR, PEG_MINOR, PEG_LEVEL);
  fprintf(output, "\n");
  fprintf(output, "%s", header);
  fprintf(output, "#define YYRULECOUNT %d\n", ruleCount);
}

int consumesInput(Node *node)
{
  if (!node) return 0;

  switch (node->type)
    {
    case Rule:
      {
	int result= 0;
	if (RuleReached & node->rule.flags)
	  fprintf(stderr, "possible infinite left recursion in rule '%s'\n", node->rule.name);
	else
	  {
	    node->rule.flags |= RuleReached;
	    result= consumesInput(node->rule.expression);
	    node->rule.flags &= ~RuleReached;
	  }
	return result;
      }
      break;

    case Dot:		return 1;
    case Name:		return consumesInput(node->name.rule);
    case Character:
    case String:	return strlen(node->string.value) > 0;
    case Class:		return 1;
    case Action:	return 0;
    case Predicate:	return 0;

    case Alternate:
      {
	Node *n;
	for (n= node->alternate.first;  n;  n= n->alternate.next)
	  if (!consumesInput(n))
	    return 0;
      }
      return 1;

    case Sequence:
      {
	Node *n;
	for (n= node->alternate.first;  n;  n= n->alternate.next)
	  if (consumesInput(n))
	    return 1;
      }
      return 0;

    case PeekFor:	return 0;
    case PeekNot:	return 0;
    case Query:		return 0;
    case Star:		return 0;
    case Plus:		return consumesInput(node->plus.element);

    default:
      fprintf(stderr, "\nconsumesInput: illegal node type %d\n", node->type);
      exit(1);
    }
  return 0;
}


void Rule_compile_c(Node *node)
{
  Node *n;

  for (n= rules;  n;  n= n->rule.next)
    consumesInput(n);

  fprintf(output, "%s", preamble);
  for (n= node;  n;  n= n->rule.next)
    fprintf(output, "YY_RULE(int) yy_%s(); /* %d */\n", n->rule.name, n->rule.id);
  fprintf(output, "\n");
  for (n= actions;  n;  n= n->action.list)
    {
      fprintf(output, "YY_ACTION(void) yy%s(char *yytext, int yyleng)\n{\n", n->action.name);
      defineVariables(n->action.rule->rule.variables);
      fprintf(output, "  yyprintf((stderr, \"do yy%s\\n\"));\n", n->action.name);
      fprintf(output, "  %s;\n", n->action.text);
      undefineVariables(n->action.rule->rule.variables);
      fprintf(output, "}\n");
    }
  Rule_compile_c2(node);
  fprintf(output, footer, start->rule.name);
}
