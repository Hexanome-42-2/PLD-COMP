#pragma once


#include "antlr4-runtime.h"
#include "generated/ifccBaseVisitor.h"
#include "StaticAnalysisVisitor.h"
#include "SymbolTable.h"
#include "IR.h"
#include <map>
#include <string>

class  CodeGenVisitor : public ifccBaseVisitor {
	public:
		CFG * cfg;
		
		CodeGenVisitor(CFG * cfg) {
			this->cfg = cfg;
		};

		virtual antlrcpp::Any visitProg(ifccParser::ProgContext *ctx) override;
		virtual antlrcpp::Any visitDeclareStatement(ifccParser::DeclareStatementContext *ctx) override;
		virtual antlrcpp::Any visitAssignStatement(ifccParser::AssignStatementContext *ctx) override;
		virtual antlrcpp::Any visitReturnStatement(ifccParser::ReturnStatementContext *ctx) override;

		// Expressions
		virtual antlrcpp::Any visitUnaryExpr(ifccParser::UnaryExprContext *ctx) override;
		virtual antlrcpp::Any visitConstExpr(ifccParser::ConstExprContext *ctx) override;
		virtual antlrcpp::Any visitVarExpr(ifccParser::VarExprContext *ctx) override;
		virtual antlrcpp::Any visitAddSub(ifccParser::AddSubContext *ctx) override;
		virtual antlrcpp::Any visitMultDiv(ifccParser::MultDivContext *ctx) override;
		virtual antlrcpp::Any visitBitWise(ifccParser::BitWiseContext *ctx) override;
		virtual antlrcpp::Any visitPar(ifccParser::ParContext *ctx) override;
};

