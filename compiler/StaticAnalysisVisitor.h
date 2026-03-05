#pragma once

#include "antlr4-runtime.h"
#include "generated/ifccBaseVisitor.h"
#include <map>
#include <string>
#include <iostream>

// Structure to store variable metadata
struct VarInfo {
	int index;	// Memory offset (multiple of 4)
	bool used;	// Whether the variable is read in an expression
};

class StaticAnalysisVisitor : public ifccBaseVisitor {
	public:
		std::map<std::string, VarInfo> symbolTable;

		int currIndex = 0;
		bool hasError = false;

		virtual std::any visitProg(ifccParser::ProgContext *ctx) override;
		virtual std::any visitDeclareStatement(ifccParser::DeclareStatementContext *ctx) override;
		virtual std::any visitAssignStatement(ifccParser::AssignStatementContext *ctx) override;
		virtual std::any visitVarExpr(ifccParser::VarExprContext *ctx) override;
};
