%{
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "parser.hpp"
using Instruction= Interpreter::Instruction;
#include "script.tab.hpp"

#define YY_DECL int yylex(Interpreter::Parser * parser)


/* 一个功能性函数-都要加 */
extern "C" {
int yywrap(void)
{
    return 1;
}
}


%}

%option yylineno

%start COMMENT STRING_LITERAL_STATE 
%%
<INITIAL>"func"      return FUNCTION;
<INITIAL>"for"       return FOR;
<INITIAL>"if"        return IF;
<INITIAL>"else if"   return ELIF;
<INITIAL>"else"      return ELSE;
<INITIAL>"+"         return ADD;
<INITIAL>"-"         return SUB;
<INITIAL>"*"         return MUL;
<INITIAL>"/"         return DIV;
<INITIAL>"="         return ASSIGN;
<INITIAL>"=="        return EQ;
<INITIAL>"!="        return NE;
<INITIAL>">"         return GT;
<INITIAL>">="        return GE;
<INITIAL>"<"         return LT;
<INITIAL>"<="        return LE;
<INITIAL>"("         return LP;
<INITIAL>")"         return RP;
<INITIAL>"{"         return LC;
<INITIAL>"}"         return RC;
<INITIAL>";"         return SEMICOLON;
<INITIAL>"break"     return BREAK;
<INITIAL>"continue"  return CONTINUE;
<INITIAL>"return"    return RETURN;
<INITIAL>","         return COMMA;
<INITIAL>"var"       return VAR;
<INITIAL>"%"         return MOD;
<INITIAL>"+="        return ADDASSIGN;
<INITIAL>"-="        return SUBASSIGN;
<INITIAL>"*="        return MULASSIGN;
<INITIAL>"/="        return DIVASSIGN;
<INITIAL>"++"        return INC;
<INITIAL>"--"        return DEC;
<INITIAL>"!"         return NOT;
<INITIAL>"["         return LB;
<INITIAL>"]"         return RB;
<INITIAL>":"         return COLON;
<INITIAL>"in"        return IN;
<INITIAL>"switch"    return SWITCH;
<INITIAL>"case"      return CASE;
<INITIAL>"default"   return DEFAULT;
<INITIAL>"|"         return BOR;
<INITIAL>"&"         return BAND;
<INITIAL>"^"         return BXOR;
<INITIAL>"~"         return BNG;
<INITIAL>"<<"        return LSHIFT;
<INITIAL>">>"        return RSHIFT;
<INITIAL>"|="        return BORASSIGN;
<INITIAL>"&="        return BANDASSIGN;
<INITIAL>"^="        return BXORASSIGN;
<INITIAL>"<<="       return LSHIFTASSIGN;
<INITIAL>">>="       return RSHIFTASSIGN;
<INITIAL>"||"        return OR;
<INITIAL>"&&"        return AND;
<INITIAL>"."         return POINT;

<INITIAL>[A-Za-z_][A-Za-z_0-9]* {
    yylval.identifier = parser->CreateString(yytext);
    return IDENTIFIER;
}
<INITIAL>([1-9][0-9]*)|"0" {
    yylval.value_integer = strtoll(yytext,NULL,10);
    return INT_LITERAL;
}
<INITIAL>0[xX][0-9a-fA-F]+ {
    yylval.value_integer = strtoll(yytext+2,NULL,16);
    return INT_LITERAL;
}
<INITIAL>[0-9]+\.[0-9]+ {
    double temp;
    sscanf(yytext, "%lf", &temp);
    yylval.value_double = temp;
    return DOUBLE_LITERAL;
}
<INITIAL>'[a-zA-Z]' {
    yylval.value_integer = yytext[1];
    return INT_LITERAL;
}

<INITIAL>\" {
    parser->StartScanningString();
    BEGIN STRING_LITERAL_STATE;
}

<INITIAL>\n ;
<INITIAL>[ \t] ;
<INITIAL>#     BEGIN COMMENT;
<INITIAL>.      {
    //error
    char buf[1024];
    if (isprint(yytext[0])) {
        buf[0] = yytext[0];
        buf[1] = '\0';
    } else {
        sprintf(buf, "0x%02x", (unsigned char)yytext[0]);
    }
    printf("meet:%s %d",buf,yylineno);
    abort();
}

<COMMENT>\n     {
    BEGIN INITIAL;
}
<COMMENT>.      ;

<STRING_LITERAL_STATE>\"        {
    yylval.identifier = parser->FinishScanningString();
    BEGIN INITIAL;
    return STRING_LITERAL;
}
<STRING_LITERAL_STATE>\n        {
    parser->AppendToScanningString('\n');
}
<STRING_LITERAL_STATE>\\r        {
    parser->AppendToScanningString('\r');
}
<STRING_LITERAL_STATE>\\\"      parser->AppendToScanningString('"');
<STRING_LITERAL_STATE>\\n       parser->AppendToScanningString('\n');
<STRING_LITERAL_STATE>\\t       parser->AppendToScanningString('\t');
<STRING_LITERAL_STATE>\\\\      parser->AppendToScanningString('\\');
<STRING_LITERAL_STATE>.         parser->AppendToScanningString(yytext[0]);

%%
