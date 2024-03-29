%{
#include <ctype.h>
#include <stdio.h>
#include <string.h>
#include "json_parser.hpp"
#include "json.tab.hpp"
#define YY_DECL int parser_json(YYSTYPE* yylval,yyscan_t yyscanner,JSONParser * parser)


%}

%option yylineno
%option reentrant

%start STRING_LITERAL_STATE COMMENT
%%
<INITIAL>"{"         return LC;
<INITIAL>"}"         return RC;
<INITIAL>","         return COMMA;
<INITIAL>"["         return LB;
<INITIAL>"]"         return RB;
<INITIAL>":"         return COLON;
<INITIAL>"true"      return TRUETOKEN;
<INITIAL>"false"     return FALSETOKEN;
<INITIAL>"null"      return NIL;

<INITIAL>\" {
    parser->StartScanningString();
    BEGIN STRING_LITERAL_STATE;
}

<INITIAL>([+-]*[1-9][0-9]*)|"0" {
    yylval->text = parser->NewString(yytext);
    return NUMBER;
}

<INITIAL>[+-]*[0-9]+\.[0-9]+ {
    yylval->text = parser->NewString(yytext);
    return NUMBER;
}

<INITIAL>\n ;
<INITIAL>[ \t] ;
<INITIAL>\r ;

<INITIAL>#     BEGIN COMMENT;
<COMMENT>\n     {
    BEGIN INITIAL;
}
<COMMENT>.      ;


<STRING_LITERAL_STATE>\"        {
    yylval->text = parser->FinishScanningString();
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
