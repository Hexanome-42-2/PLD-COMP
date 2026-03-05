#pragma once


#include "antlr4-runtime.h"
#include "generated/ifccBaseVisitor.h"
#include "StaticAnalysisVisitor.h"
#include <map>
#include <string>

class  CodeGenVisitor : public ifccBaseVisitor {
	public:
		std::map<std::string, VarInfo> symbolTable;
		
		CodeGenVisitor(std::map<std::string, VarInfo> st) : symbolTable(st) {};

		virtual antlrcpp::Any visitProg(ifccParser::ProgContext *ctx) override ;
		virtual antlrcpp::Any visitDeclareStatement(ifccParser::DeclareStatementContext *ctx) override;
		virtual antlrcpp::Any visitAssignStatement(ifccParser::AssignStatementContext *ctx) override;
		virtual antlrcpp::Any visitReturnStatement(ifccParser::ReturnStatementContext *ctx) override;

		// Expressions
		virtual antlrcpp::Any visitConstExpr(ifccParser::ConstExprContext *ctx) override;
		virtual antlrcpp::Any visitVarExpr(ifccParser::VarExprContext *ctx) override;
};

