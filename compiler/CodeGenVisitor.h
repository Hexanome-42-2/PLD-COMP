#pragma once


#include "antlr4-runtime.h"
#include "generated/ifccBaseVisitor.h"
#include "StaticAnalysisVisitor.h"
#include "SymbolTable.h"
#include <map>
#include <string>

class  CodeGenVisitor : public ifccBaseVisitor {
	public:
		SymbolTable *symbolTable;
		
		CodeGenVisitor(SymbolTable *table) {
			symbolTable = table;
		};

		virtual antlrcpp::Any visitProg(ifccParser::ProgContext *ctx) override ;
		virtual antlrcpp::Any visitDeclareStatement(ifccParser::DeclareStatementContext *ctx) override;
		virtual antlrcpp::Any visitAssignStatement(ifccParser::AssignStatementContext *ctx) override;
		virtual antlrcpp::Any visitReturnStatement(ifccParser::ReturnStatementContext *ctx) override;

		// Expressions
		virtual antlrcpp::Any visitUnaryExpr(ifccParser::UnaryExprContext *ctx) override;
		virtual antlrcpp::Any visitConstExpr(ifccParser::ConstExprContext *ctx) override;
		virtual antlrcpp::Any visitVarExpr(ifccParser::VarExprContext *ctx) override;
		virtual antlrcpp::Any visitAdd(ifccParser::AddContext *ctx) override;
		virtual antlrcpp::Any visitMult(ifccParser::MultContext *ctx) override;
		virtual antlrcpp::Any visitPar(ifccParser::ParContext *ctx) override;
};

