grammar ifcc;

axiom : prog EOF ;

prog 		: ( fonction | statement )* mainFunc ( fonction | statement )*;

fonction  	: (INT_TYPE | VOID_TYPE) funcName=VAR '(' parameters? ')' '{' ( (statement | return_stmt)* RETURN expr? ';' )* '}'		# Function
			;

mainFunc  	: INT_TYPE 'main' '(' ')' '{' ( (statement | return_stmt)* RETURN expr? ';' )* '}'		# MainFunction
			;

// 1. Defines function parameters (each param has explicit type: int a, int b)
parameters	: INT_TYPE VAR (',' INT_TYPE VAR)*		# ParamList
			;

// 2. Defines function call arguments
argument	: expr (',' expr)*						# ArgumentList
			;

// 3. Defines what a statement is
statement	: INT_TYPE VAR '=' expr ';'				# DeclareAssignStatement
			| INT_TYPE VAR (',' VAR)* ';'		    # DeclareStatement
			| VAR '=' expr ';'				    	# AssignStatement
			| VAR '(' argument? ')' ';'				# FunctionCallStatement
			;

return_stmt	: RETURN expr ';'                   	# ReturnStatement
			;

// 4. Defines what an expression is
expr		: ( NEG )? expr_unary			    	# UnaryExpr
			| lExpr=expr MULTOP rExpr=expr			# MultDiv
			| lExpr=expr ADDOP rExpr=expr			# AddSub
			;

expr_unary	: CONST							    	# ConstExpr
			| VAR '(' argument? ')'					# FuncCallExpr
			| VAR							    	# VarExpr
			| '(' expr ')'					    	# Par
			;

expr_bool   : expr                              	# Bool
            | expr COMPOP expr                  	# Comp
            | expr EQOP expr                    	# EQ
            ;

// ~~~~~~~~~~ LEXER Rules (Tokens) ~~~~~~~~~~ //

NEG			: '-' | '!' ;
ADDOP       : '+' | '-' ;
MULTOP      : '*' | '/' | '%' ;
BITOP       : '&' | '|' | '^' ;
COMPOP      : '<' | '>' ;
EQOP        : '==' | '!=' ;
RETURN		: 'return' ;
INT_TYPE	: 'int' ;
VOID_TYPE	: 'void' ;
VAR			: [a-zA-Z_] [a-zA-Z0-9_]* ;
CONST 		: [0-9]+ ;
COMMENT 	: '/*' .*? '*/' -> skip ;
LINE_COMMENT: '//' ~[\r\n]* -> skip ;
DIRECTIVE 	: '#' .*? '\n' -> skip ;
WS    		: [ \t\r\n] -> channel(HIDDEN);
