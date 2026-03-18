grammar ifcc;

axiom : prog EOF ;

prog		: 'int' 'main' '(' ')' '{' statement+ '}' ;

// 2. Defines what a statement is
statement	: 'int' VAR (',' VAR)* ';'		        # DeclareStatement
			| VAR '=' expr ';'				        # AssignStatement
			| RETURN expr ';'                       # ReturnStatement
			;

// 3. Defines what an expression is
expr		: lExpr=expr MULTOP rExpr=expr		    # MultDiv
			| lExpr=expr op=('-'|'+') rExpr=expr    # AddSub
			| ( op=('-'|'+'|'!') )? expr_unary      # UnaryExpr
            ;

expr_unary	: CONST							        # ConstExpr
			| VAR							        # VarExpr
			| '(' expr ')'					        # Par
			;

expr_bool   : expr                                  # Bool
            | expr COMPOP expr                      # Comp
            | expr EQOP expr                        # EQ
            ;

// ~~~~~~~~~~ LEXER Rules (Tokens) ~~~~~~~~~~ //

MULTOP      : '*' | '/' | '%' ;
BITOP       : '&' | '|' | '^' ;
COMPOP      : '<' | '>' ;
EQOP        : '==' | '!=' ;
RETURN		: 'return' ;
VAR			: [a-zA-Z_] [a-zA-Z0-9_]* ;
CONST 		: [0-9]+ ;
COMMENT 	: '/*' .*? '*/' -> skip ;
DIRECTIVE 	: '#' .*? '\n' -> skip ;
WS    		: [ \t\r\n] -> channel(HIDDEN);
