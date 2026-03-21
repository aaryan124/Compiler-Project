
/* A Bison parser, made by GNU Bison 2.4.1.  */

/* Skeleton interface for Bison's Yacc-like parsers in C
   
      Copyright (C) 1984, 1989, 1990, 2000, 2001, 2002, 2003, 2004, 2005, 2006
   Free Software Foundation, Inc.
   
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.
   
   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.
   
   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.  */

/* As a special exception, you may create a larger work that contains
   part or all of the Bison parser skeleton and distribute that work
   under terms of your choice, so long as that work isn't itself a
   parser generator using the skeleton or a modified version thereof
   as a parser skeleton.  Alternatively, if you modify or redistribute
   the parser skeleton itself, you may (at your option) remove this
   special exception, which will cause the skeleton and the resulting
   Bison output files to be licensed under the GNU General Public
   License without this special exception.
   
   This special exception was added by the Free Software Foundation in
   version 2.2 of Bison.  */


/* Tokens.  */
#ifndef YYTOKENTYPE
# define YYTOKENTYPE
   /* Put the tokens into the symbol table, so that GDB and other debuggers
      know about them.  */
   enum yytokentype {
     T_INT = 258,
     T_VOID = 259,
     T_IF = 260,
     T_ELSE = 261,
     T_WHILE = 262,
     T_RETURN = 263,
     T_PRINTF = 264,
     T_SCANF = 265,
     T_PLUS = 266,
     T_MINUS = 267,
     T_STAR = 268,
     T_SLASH = 269,
     T_ASSIGN = 270,
     T_EQ = 271,
     T_NEQ = 272,
     T_LT = 273,
     T_GT = 274,
     T_LTE = 275,
     T_GTE = 276,
     T_AND = 277,
     T_OR = 278,
     T_NOT = 279,
     T_LPAREN = 280,
     T_RPAREN = 281,
     T_LBRACE = 282,
     T_RBRACE = 283,
     T_LBRACKET = 284,
     T_RBRACKET = 285,
     T_SEMICOLON = 286,
     T_COMMA = 287,
     T_IDENTIFIER = 288,
     T_INT_LITERAL = 289,
     LOWER_THAN_ELSE = 290
   };
#endif



#if ! defined YYSTYPE && ! defined YYSTYPE_IS_DECLARED
typedef union YYSTYPE
{

/* Line 1676 of yacc.c  */
#line 19 "parser.y"

    int intval;
    char* str;
    ASTNode* node;



/* Line 1676 of yacc.c  */
#line 95 "parser.tab.h"
} YYSTYPE;
# define YYSTYPE_IS_TRIVIAL 1
# define yystype YYSTYPE /* obsolescent; will be withdrawn */
# define YYSTYPE_IS_DECLARED 1
#endif

extern YYSTYPE yylval;


