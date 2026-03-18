grammar ifcc;

axiom : prog EOF ;

prog 		: fonction+;

fonction  	: functype=TYPE funcName=NAME '(' parameters? ')' block		# Function
			;

block		: '{' statement* '}'				# BlockStatement
			;

// 1. Defines function parameters
parameters	: TYPE NAME (',' TYPE NAME)*		# ParamList
			;

// 2. Defines what a statement is
statement	: TYPE NAME (',' NAME)* ';'		# DeclareStatement
			| NAME '=' expr ';'				    # AssignStatement
			| RETURN expr ';'                   # ReturnStatement
			;

// 3. Defines what an expression is
expr		: ( NEG )? expr_unary			    # UnaryExpr
			| NAME '(' (expr (',' expr)*)? ')'	# FuncCall
			| lExpr=expr MULTOP rExpr=expr		# MultDiv
			| lExpr=expr ADDOP rExpr=expr		# AddSub
			;
	
expr_unary	: CONST							    # ConstExpr
			| NAME							    # VarExpr
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
TYPE		: 'int' ;
NAME		: [a-zA-Z_] [a-zA-Z0-9_]* ;
CONST 		: [0-9]+ ;
COMMENT 	: '/*' .*? '*/' -> skip ;
DIRECTIVE 	: '#' .*? '\n' -> skip ;
WS    		: [ \t\r\n] -> channel(HIDDEN);
