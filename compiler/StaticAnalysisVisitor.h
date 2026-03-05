#pragma once

#include "antlr4-runtime.h"
#include "generated/ifccBaseVisitor.h"
#include "SymbolTable.h"
#include <iostream>

class StaticAnalysisVisitor : public ifccBaseVisitor {
	public:
		SymbolTable *symbolTable;

		int currIndex = 0;
		bool hasError = false;

		StaticAnalysisVisitor(SymbolTable *table) {
			symbolTable = table;
		};
		~StaticAnalysisVisitor() = default;

		virtual std::any visitProg(ifccParser::ProgContext *ctx) override;
		virtual std::any visitDeclareStatement(ifccParser::DeclareStatementContext *ctx) override;
		virtual std::any visitAssignStatement(ifccParser::AssignStatementContext *ctx) override;
		virtual std::any visitVarExpr(ifccParser::VarExprContext *ctx) override;
};
