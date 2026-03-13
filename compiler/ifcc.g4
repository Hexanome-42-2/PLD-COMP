grammar ifcc;

axiom : prog EOF ;

prog 		: fonctions mainFunc fonctions;

fonctions  	: (FUNCTYPE VAR '(' parameters? ')' '{' ( statement* RETURN expr? ';' )* '}' )*		# FunctionDefinition
			;

mainFunc  	: 'int' 'main' '(' ')' '{' ( statement* RETURN expr? ';' )* '}'		# MainFunction
			;
			
// 1. Defines function parameters
parameters	: 'int' VAR (',' VAR)*			# ParamList
			;

// 2. Defines what a statement is
statement	: 'int' VAR (',' VAR)* ';'		    # DeclareStatement
			| VAR '=' expr ';'				    # AssignStatement
			| RETURN expr ';'                   # ReturnStatement
			;

// 3. Defines what an expression is
expr		: ( NEG )? expr_unary			    # UnaryExpr
			| lExpr=expr MULTOP rExpr=expr		# MultDiv
			| lExpr=expr ADDOP rExpr=expr		# AddSub
			;
	
expr_unary	: CONST							    # ConstExpr
			| VAR							    # VarExpr
			| '(' expr ')'					    # Par
			;

expr_bool   : expr                              # Bool
            | expr COMPOP expr                  # Comp
            | expr EQOP expr                    # EQ
            ;

// ~~~~~~~~~~ LEXER Rules (Tokens) ~~~~~~~~~~ //

NEG			: '-' | '!' ;
ADDOP       : '+' | '-' ;
MULTOP      : '*' | '/' | '%' ;
BITOP       : '&' | '|' | '^' ;
COMPOP      : '<' | '>' ;
EQOP        : '==' | '!=' ;
RETURN		: 'return' ;
VAR			: [a-zA-Z_] [a-zA-Z0-9_]* ;
FUNCTYPE	: 'int' | 'void' ;
CONST 		: [0-9]+ ;
COMMENT 	: '/*' .*? '*/' -> skip ;
DIRECTIVE 	: '#' .*? '\n' -> skip ;
WS    		: [ \t\r\n] -> channel(HIDDEN);
