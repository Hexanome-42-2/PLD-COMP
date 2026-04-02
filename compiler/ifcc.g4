grammar ifcc;

axiom : prog EOF ;

prog 		: ( include | fonction )+ ;

include		: '#' INCLUDE ( file=HEADER_LOCAL  | file=HEADER_LIB )		# IncludeStatement
			;

fonction  	: functype=TYPE funcName=NAME '(' parameters? ')' ';'		# FunctionDeclaration
			| functype=TYPE funcName=NAME '(' parameters? ')' block		# FunctionDefinition
			;

block		: '{' ( include | statement )* '}'
			;

// 1. Defines function parameters (each param has explicit type: int a, int b)
parameters	: TYPE NAME (',' TYPE NAME)*		# ParamList
			;

// 2. Defines function call arguments
argument	: expr (',' expr)*					# ArgumentList
			;

// 3. Defines what a statement is
statement	: block                                             # BlockStatement
            | TYPE assignStatement (',' assignStatement)* ';'	# DeclareStatement
			| NAME '(' argument? ')' ';'		                # FunctionCallStatement
			| RETURN expr? ';'                                  # ReturnStatement
			| 'if' '(' expr ')' ( ifst=statement | ifbl=block )
			  ('else' ( elst=statement | elbl=block ))?         # IfStatement
			| 'while' '(' expr ')'
			  ( whst=statement | whbl=block )                   # WhileStatement
			| expr ';'								            # ExprStatement
			;

assignStatement : NAME ('=' expr)? ;

// 4. Defines what an expression is
expr		: lExpr=expr MULTOP rExpr=expr		    # MultDiv
			| lExpr=expr op=('-'|'+') rExpr=expr    # AddSub
			| ( op=('-'|'+'|'!') )? expr_unary      # UnaryExpr
			| lExpr=expr BITOP rExpr=expr		    # BitWise
            | lExpr=expr COMPOP rExpr=expr          # Comp
            | lExpr=expr EQOP rExpr=expr            # EQ
            | assignStatement                       # AssignExpr
            ;

expr_unary	: CONST							    # ConstExpr
			| CHAR_CONST					    # CharConstExpr
			| NAME '(' argument? ')'			# FuncCall
			| NAME							    # VarExpr
			| '(' expr ')'					    # Par
			;

// ~~~~~~~~~~ LEXER Rules (Tokens) ~~~~~~~~~~ //

COMMENT 			: ('/*' .*? '*/' | '//' .*? '\n') -> skip ;
MULTOP      		: '*' | '/' | '%' ;
BITOP       		: '&' | '|' | '^' ;
COMPOP      		: '<' | '>' ;
EQOP       	 		: '==' | '!=' ;
INCLUDE			 	: 'include' ;
HEADER_LIB			: '<' [a-zA-Z0-9_./-]+ '>' ;
HEADER_LOCAL		: '"' ~["\r\n]+ '"' ;
RETURN				: 'return' ;
TYPE				: 'int' | 'void' ;
NAME				: [a-zA-Z_] [a-zA-Z0-9_]* ;
CONST 				: [0-9]+ ;
CHAR_CONST	        : '\'' ( '\\' . | ~['\\] ) '\'' ;
WS    				: [ \t\r\n] -> channel(HIDDEN);
ANY					: .  -> skip ;