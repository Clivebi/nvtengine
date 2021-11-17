%parse-param {JSONParser * parser}
%lex-param {JSONParser * parser}
%{
#include "json_parser.hpp"
%}
%require "3.2"
%{
// https://stackoverflow.com/questions/23717039/generating-a-compiler-from-lex-and-yacc-grammar
//int yylex(JSONParser * parser);
int parser_json(JSONParser * parser);
void parser_json_error(JSONParser * parser,const char *s);
#define yylex   parser_json
#define yyerror parser_json_error
//void yyerror(JSONParser * parser,const char *s);
%}
%union {
    const char* text;
    JSONValue*       object;
    JSONMember*      member;
};


%token  <text> NUMBER STRING_LITERAL LC RC COMMA LB RB COLON TRUE FALSE NIL

%type <object> root value value_list object_member_list object array
%type <member> object_member
%%

%start  root;

root: object
      {
              parser->root = $1;
              $$= $1;
      }
      |array{
              parser->root = $1;
              $$= $1;
      }
      ;


value: NUMBER
        {
                $$=parser->NewValue($1);
                $$->Type = JSONValue::NUMBER;
        }
       |STRING_LITERAL
       {
                $$=parser->NewValue($1);
        }
       |TRUE
       {
                $$=parser->NewValue($1);
                $$->Type = JSONValue::BOOL;
        }
       |FALSE
       {
                $$=parser->NewValue($1);
                $$->Type = JSONValue::BOOL;
        }
       |NIL{
               $$=parser->NewValue($1);
               $$->Type = JSONValue::NIL;
       }
       |object{
               $$=$1;
       }
       |array{
               $$=$1;
       }
       ;

value_list:value_list COMMA value
        {
                $$=parser->AddToArray($1,$3);
        }
        |value
        {
                $$=parser->NewArray($1);
        }
        ;

array: LB RB 
        {
                $$=parser->NewArray(NULL);
        }
        | LB value_list RB
        {
                $$=$2;
        }
        ;

object_member:STRING_LITERAL COLON value
        {
                $$=parser->NewMember($1,$3);
        }
        ;

object_member_list:object_member_list COMMA object_member
        {       
                $$=parser->AddMember($1,$3);
        }
        |object_member{
                $$=parser->NewObject($1);
        }
        ;

object: LC object_member_list RC
        {
                $$=$2;
        }
        |LC RC
        {
                $$=parser->NewObject(NULL);
        }
        ;



