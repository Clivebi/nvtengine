%parse-param {Interpreter::Parser * parser}
%lex-param {Interpreter::Parser * parser}
%{
#include <stdio.h>
#include <string>
#include "parser.hpp"
%}
%require "3.2"
%{
//using Instruction= Interpreter::Instruction;
using namespace Interpreter;
int yylex(Interpreter::Parser * parser);
void yyerror(Interpreter::Parser * parser,const char *s);
%}
%union {
    const char* identifier;
    long        value_integer;
    double      value_double; 
    Instruction* object;
};

%token  <identifier>VAR FUNCTION FOR IF ELIF ELSE ADD SUB MUL DIV MOD ASSIGN
        EQ NE GT GE LT LE LP RP LC RC SEMICOLON IDENT
        BREAK CONTINUE RETURN COMMA STRING_LITERAL COLON ADDASSIGN SUBASSIGN
        MULASSIGN DIVASSIGN INC DEC NOT LB RB IN SWITCH CASE DEFAULT
        BOR BAND BXOR BNG LSHIFT RSHIFT  BORASSIGN BANDASSIGN BXORASSIGN 
        LSHIFTASSIGN RSHIFTASSIGN OR AND POINT MOREVAL URSHIFT URSHIFTASSIGN

%token <value_integer> INT_LITERAL
%token <value_double>  DOUBLE_LITERAL

%left  COMMA
%right ASSIGN ADDASSIGN SUBASSIGN DIVASSIGN MULASSIGN
       BORASSIGN BANDASSIGN BXORASSIGN LSHIFTASSIGN RSHIFTASSIGN URSHIFTASSIGN
       LB RB
%left OR
%left AND 
%left BOR
%left BXOR
%left BAND
%left NE
%left EQ 


%left  GT GE LT LE 
%left  LSHIFT RSHIFT URSHIFT
%left  ADD SUB
%left  MUL DIV MOD
%left  COLON
%left LP RP 
%right NOT INC DEC BNG 

%nonassoc '|' UMINUS 

%type <identifier> IDENTIFIER
%type <object> declaration declarationlist var_declaration

%type <object> assign_expression assign_expression_list

%type <object> const_value lvalue rvalue  binary_expression unary_expression
%type <object> slice 
%type <object> map array map_pair map_pair_list
%type <object> named_value named_value_list
%type <object> value_indexer_path object_indexer  read_lvalue indexer

%type <object> func_declaration func_call_expression return_expression
%type <object> formal_parameter formal_parameterlist value_list condition_value

%type <object> statementlist statement
%type <object> statement_in_block_list statement_in_block block
%type <object> condition_statement if_expresion elseif_expresion elseif_expresionlist else_expresion

%type <object> for_init for_statement for_condition for_update
%type <object> break_expression continue_expression 
%type <object> for_in_statement
%type <object> case_item case_item_list switch_case_statement const_value_list

%%

%start  startstatement;


declaration: IDENTIFIER
        {
                $$= parser->VarDeclarationExpresion($1,NULL);
        }
        | IDENTIFIER ASSIGN rvalue
        {
                $$= parser->VarDeclarationExpresion($1,$3);
        }
        ;

IDENTIFIER:IDENT
        {
                $$= $1;
        }
        |IN 
        {
                $$="in";
        };

declarationlist:declarationlist COMMA declaration
        {
                $$= parser->AddToList($1,$3);
        }
        | declaration
        {
                $$= parser->CreateList(KnownListName::kDecl,$1); 
        }
        ;

var_declaration: VAR declarationlist
        {
                $$=$2;
        }
        ;


block: LC statement_in_block_list RC 
        {
                $$=$2;
        }
        | LC RC 
        {
             $$=parser->NULLObject();    
        }
        ;

condition_statement:if_expresion
        {
                $$=$1;
        }
        |if_expresion  else_expresion
        {
                $$=parser->CreateIFStatement($1,NULL,$2);
        }
        |if_expresion  elseif_expresionlist  else_expresion
        {
                $$=parser->CreateIFStatement($1,$2,$3);
        }
        |if_expresion elseif_expresionlist
        {
                $$=parser->CreateIFStatement($1,$2,NULL);
        }
        ;

condition_value:rvalue
        |assign_expression
        ;

if_expresion: IF LP condition_value RP block
        {
                $$=parser->CreateConditionExpresion($3,$5);
        }
        ;

elseif_expresionlist:elseif_expresionlist  elseif_expresion
        {
                $$=parser->AddToList($1,$2);
        }
        |elseif_expresion
        {
                $$= parser->CreateList(KnownListName::kElseIf,$1); 
        }
        ;

elseif_expresion: ELIF LP condition_value RP block
        {
                $$=parser->CreateConditionExpresion($3,$5);
        }
        ;

else_expresion: ELSE block
        {
                $$= $2;
        }
        ;

break_expression:BREAK
        {
                $$=parser->CreateBreakStatement();
        }
        ;

continue_expression:CONTINUE
        {
                $$=parser->CreateContinueStatement();
        }
        ;

for_init: var_declaration
        |assign_expression_list
        |func_call_expression
        |unary_expression
        |
        {
                $$=parser->NULLObject();
        }
        ;

for_condition: rvalue
        |assign_expression
        |
        {
                $$=parser->NULLObject();
        }
        ;

for_update:assign_expression_list
        |func_call_expression
        |unary_expression
        |
        {
                $$=parser->NULLObject();
        }
        ;


for_statement: FOR LP for_init SEMICOLON for_condition SEMICOLON for_update RP block
        {
                $$=parser->CreateForStatement($3,$5,$7,$9);
        }
        |FOR block
        {
                $$=parser->CreateForStatement(NULL,NULL,NULL,$2);
        }
        ;

for_in_statement:FOR IDENTIFIER COMMA IDENTIFIER IN rvalue block
        {
                $$=parser->CreateForInStatement($2,$4,$6,$7);
        }
        | FOR IDENTIFIER IN rvalue block
        {
                $$=parser->CreateForInStatement("",$2,$4,$5);
        }
        ;


const_value:INT_LITERAL
        {
                $$= parser->CreateConst($1);
        }
        |DOUBLE_LITERAL
        {
                $$= parser->CreateConst($1);
        }
        |STRING_LITERAL
        {
                $$= parser->CreateConst($1);
        }
        ;

const_value_list:const_value_list COMMA const_value
        {
                $$=parser->AddToList($1,$3);        
        }
        |const_value
        {
                $$= parser->CreateList(KnownListName::kConst,$1); 
        }
        ;

return_expression: RETURN rvalue
        {
                $$= parser->CreateReturnStatement($2);
        }
        | RETURN
        {
                $$= parser->CreateReturnStatement(NULL); 
        }
        ;

func_declaration:FUNCTION IDENTIFIER LP formal_parameterlist RP block
        {
                $$=parser->CreateFunction($2,$4,$6);
        }
        | FUNCTION IDENTIFIER LP RP block
        {
                $$=parser->CreateFunction($2,NULL,$5);
        }
        | FUNCTION IDENTIFIER LP IDENTIFIER MOREVAL RP block
        {
                Instruction* obj =parser->VarDeclarationExpresion($4,NULL);
                obj= parser->CreateList(KnownListName::kDeclMore,obj); 
                $$=parser->CreateFunction($2,obj,$7);
        }
        ;

func_call_expression: IDENTIFIER LP value_list RP
        {
                $$=parser->CreateFunctionCall($1,$3);
        }
        | IDENTIFIER LP named_value_list RP{
                $$=parser->CreateFunctionCall($1,$3);
        }
        | IDENTIFIER LP RP
        {
                $$=parser->CreateFunctionCall($1,NULL);
        }
        ;


formal_parameterlist:formal_parameterlist COMMA formal_parameter
        {
            $$=parser->AddToList($1,$3);      
        }
        |formal_parameter
        {
             $$= parser->CreateList(KnownListName::kDeclFuncArgs,$1); 
        }
        ;

formal_parameter:IDENTIFIER
        {
                $$=parser->VarDeclarationExpresion($1,NULL);
        }
        ;


value_list:value_list COMMA rvalue
        {
                $$=parser->AddToList($1,$3);    
        }
        |rvalue
        {
                $$= parser->CreateList(KnownListName::kValue,$1); 
        }
        ;

named_value:IDENTIFIER COLON rvalue
        {
                $$= parser->VarDeclarationExpresion($1,$3);
        }
        ;

named_value_list:named_value_list COMMA named_value
        {
                $$=parser->AddToList($1,$3);
        }
        |named_value
        {
                $$=parser->CreateList(KnownListName::kNamedValue,$1);
        };

rvalue:indexer
        |map
        |array
        |slice
        |SUB rvalue %prec UMINUS
        {
                $$=parser->CreateMinus($2);
        }
        |LP SUB rvalue %prec UMINUS RP 
        {
                $$=parser->CreateMinus($3);
        }
        ;

indexer:const_value 
        |func_call_expression
        |binary_expression
        |unary_expression
        |read_lvalue
        ;

value_indexer_path:LB indexer RB
        { 
                $$=parser->CreateList(KnownListName::kIndexer,$2);   
        }
        |value_indexer_path LB indexer RB
        {
                $$=parser->AddToList($1,$3);
        }
        ;

object_indexer: POINT IDENTIFIER
        {
                $$=parser->CreateObjectIndexer($2);
        }
        | object_indexer POINT IDENTIFIER
        {
                $$=parser->AddObjectIndexer($1,$3);
        }
        ;
        

lvalue:IDENTIFIER
        {
                $$=parser->CreateReference($1,NULL);
        }
        | IDENTIFIER value_indexer_path
        {
                $$=parser->CreateReference($1,$2);
        }
        | IDENTIFIER object_indexer
        {
                $$=parser->CreateReference($1,$2);
        }
        ;

read_lvalue:IDENTIFIER
        {
                $$=parser->VarReadExpresion($1);
        }
        | IDENTIFIER value_indexer_path
        {
                $$=parser->VarReadReference($1,$2);
        }
        | IDENTIFIER object_indexer
        {
                $$=parser->VarReadReference($1,$2);
        }
        ;


binary_expression: rvalue ADD rvalue
        {
                $$=parser->CreateBinaryOperation($1,$3,Interpreter::Instructions::kADD);
        }
        | rvalue SUB rvalue
        {
                $$=parser->CreateBinaryOperation($1,$3,Interpreter::Instructions::kSUB);
        }
        | rvalue MUL rvalue
        {
                $$=parser->CreateBinaryOperation($1,$3,Interpreter::Instructions::kMUL);
        }
        | rvalue DIV rvalue
        {
                $$=parser->CreateBinaryOperation($1,$3,Interpreter::Instructions::kDIV);
        }
        | rvalue MOD rvalue
        {
                $$=parser->CreateBinaryOperation($1,$3,Interpreter::Instructions::kMOD);
        }
        |rvalue BAND rvalue
        {
                $$=parser->CreateBinaryOperation($1,$3,Interpreter::Instructions::kBAND);
        }
        |rvalue BOR rvalue
        {
                $$=parser->CreateBinaryOperation($1,$3,Interpreter::Instructions::kBOR);
        }
        |rvalue BXOR rvalue
        {
                $$=parser->CreateBinaryOperation($1,$3,Interpreter::Instructions::kBXOR);
        }
        |BNG rvalue
        {
                $$=parser->CreateBinaryOperation($2,NULL,Interpreter::Instructions::kBNG);
        }
        |rvalue LSHIFT rvalue
        {
                $$=parser->CreateBinaryOperation($1,$3,Interpreter::Instructions::kLSHIFT);
        }
        |rvalue RSHIFT rvalue
        {
                $$=parser->CreateBinaryOperation($1,$3,Interpreter::Instructions::kRSHIFT);
        }
        |rvalue URSHIFT rvalue
        {
                $$=parser->CreateBinaryOperation($1,$3,Interpreter::Instructions::kURSHIFT);
        }
        |rvalue EQ rvalue
        {
                $$=parser->CreateBinaryOperation($1,$3,Interpreter::Instructions::kEQ);
        }
        |rvalue NE rvalue
        {
                $$=parser->CreateBinaryOperation($1,$3,Interpreter::Instructions::kNE);
        }
        |rvalue GT rvalue
        {
                $$=parser->CreateBinaryOperation($1,$3,Interpreter::Instructions::kGT);
        }
        |rvalue GE rvalue
        {
                $$=parser->CreateBinaryOperation($1,$3,Interpreter::Instructions::kGE);
        }
        |rvalue LT rvalue
        {
                $$=parser->CreateBinaryOperation($1,$3,Interpreter::Instructions::kLT);
        }
        |rvalue LE rvalue
        {
                $$=parser->CreateBinaryOperation($1,$3,Interpreter::Instructions::kLE);
        }
        |NOT rvalue
        {
                $$=parser->CreateBinaryOperation($2,NULL,Interpreter::Instructions::kNOT);
        }
        |rvalue OR rvalue
        {
                $$=parser->CreateBinaryOperation($1,$3,Interpreter::Instructions::kOR);
        }
        |rvalue AND rvalue
        {
                $$=parser->CreateBinaryOperation($1,$3,Interpreter::Instructions::kAND);
        }
        | LP binary_expression RP 
        {
                $$=$2;
        }
        ;


assign_expression: lvalue ASSIGN rvalue
        {       
                $$=parser->VarUpdateExpression($1,$3,Interpreter::Instructions::kuWrite);
        }
        |lvalue ADDASSIGN rvalue
        {
                $$=parser->VarUpdateExpression($1,$3,Interpreter::Instructions::kuADD);
        }
        |lvalue SUBASSIGN rvalue
        {
                $$=parser->VarUpdateExpression($1,$3,Interpreter::Instructions::kuSUB);
        }
        |lvalue MULASSIGN rvalue
        {
                $$=parser->VarUpdateExpression($1,$3,Interpreter::Instructions::kuMUL);
        }
        |lvalue DIVASSIGN rvalue
        {
                $$=parser->VarUpdateExpression($1,$3,Interpreter::Instructions::kuDIV);
        }
        |lvalue BANDASSIGN rvalue
        {
                $$=parser->VarUpdateExpression($1,$3,Interpreter::Instructions::kuBAND);
        }
        |lvalue BORASSIGN rvalue
        {
                $$=parser->VarUpdateExpression($1,$3,Interpreter::Instructions::kuBOR);
        }
        |lvalue BXORASSIGN rvalue
        {
                $$=parser->VarUpdateExpression($1,$3,Interpreter::Instructions::kuBXOR);
        }
        |lvalue LSHIFTASSIGN rvalue
        {
                $$=parser->VarUpdateExpression($1,$3,Interpreter::Instructions::kuLSHIFT);
        }
        |lvalue RSHIFTASSIGN rvalue
        {
                $$=parser->VarUpdateExpression($1,$3,Interpreter::Instructions::kuRSHIFT);
        }
        |lvalue URSHIFTASSIGN rvalue
        {
                $$=parser->VarUpdateExpression($1,$3,Interpreter::Instructions::kuURSHIFT);
        }
        ;


unary_expression: lvalue INC
        {
                $$ = parser->VarUpdateExpression($1,NULL,Interpreter::Instructions::kuINCReturnOld);
        }
        |lvalue DEC
        {
                $$ = parser->VarUpdateExpression($1,NULL,Interpreter::Instructions::kuDECReturnOld);
        }
        |INC lvalue
        {
                $$ = parser->VarUpdateExpression($2,NULL,Interpreter::Instructions::kuINC);
        }
        |DEC lvalue
        {
                $$ = parser->VarUpdateExpression($2,NULL,Interpreter::Instructions::kuDEC);
        }
        |LP unary_expression RP 
        {
                $$=$2;
        }
        ;

assign_expression_list: assign_expression_list COMMA assign_expression
        {
               $$=parser->AddToList($1,$3);  
        }
        |assign_expression
        {
               $$= parser->CreateList(KnownListName::kAssign,$1);   
        }
        ;


map_pair:rvalue COLON rvalue
        {
                $$=parser->CreateMapItem($1,$3);
        }
        ;
map_pair_list:map_pair_list COMMA map_pair
        {
                $$=parser->AddToList($1,$3); 
        }
        |map_pair
        {
                $$= parser->CreateList(KnownListName::kMapValue,$1);   
        }
        ;

array:LB value_list RB
        {
                $$=parser->CreateArray($2);
        }
        |LB RB
        {
                $$=parser->CreateArray(NULL);
        }
        ;

map: LC map_pair_list RC
        {
                $$=parser->CreateMap($2);
        }
        | LC RC
        {
                $$=parser->CreateMap(NULL);
        }
        ;

slice:IDENTIFIER LB rvalue COLON rvalue RB
        {
                $$=parser->VarSlice($1,$3,$5);
        }
        |IDENTIFIER LB rvalue COLON RB
        {
                $$=parser->VarSlice($1,$3,NULL);
        }
        |IDENTIFIER LB COLON rvalue RB
        {
                $$=parser->VarSlice($1,NULL,$4);
        }
        ;

case_item:CASE const_value_list COLON block
        {
                Instruction* obj = parser->CreateList(KnownListName::kCaseItem,$2); 
                $$=parser->AddToList(obj,$4);
        }
        ;
case_item_list:case_item_list case_item
        {
                $$=parser->AddToList($1,$2);  
        }
        |case_item
        {
                $$= parser->CreateList(KnownListName::kCase,$1); 
        }
        ;

switch_case_statement: SWITCH LP rvalue RP LC case_item_list DEFAULT COLON block RC
        {
                $$=parser->CreateSwitchCaseStatement($3,$6,$9);
        }
        |SWITCH LP rvalue RP LC case_item_list RC
        {
                $$=parser->CreateSwitchCaseStatement($3,$6,NULL);
        }
        ;


statement_in_block:var_declaration SEMICOLON
        |assign_expression SEMICOLON
        |unary_expression SEMICOLON
        |condition_statement
        |return_expression SEMICOLON
        |func_call_expression SEMICOLON
        |for_statement
        |break_expression SEMICOLON
        |continue_expression SEMICOLON
        |for_in_statement
        |switch_case_statement
        ;

statement_in_block_list:statement_in_block_list statement_in_block
        {
                $$=parser->AddToList($1,$2);
        }
        |statement_in_block
        {
                $$=parser->CreateList(KnownListName::kBlockStatement,$1); 
        }
        ;

statement: func_declaration
        |statement_in_block
        ;

statementlist: statementlist  statement
        {
                $$=parser->AddToList($1,$2);
        }
        |statement
        {
                $$=parser->CreateList(KnownListName::kStatement,$1); 
        }
        ;

startstatement:statementlist
        {
                parser->SetEntryPoint($1);
        }
        ;