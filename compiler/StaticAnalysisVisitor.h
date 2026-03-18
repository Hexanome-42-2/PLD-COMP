#pragma once

#include "antlr4-runtime.h"
#include "generated/ifccBaseVisitor.h"
#include "SymbolTable.h"
#include <iostream>
#include <unordered_map>
#include <string>

struct FunctionSignature {
	std::string returnType; // "int" or "void"
	int paramCount;
};

class StaticAnalysisVisitor : public ifccBaseVisitor {
	public:
		SymbolTable *symbolTable;

		int currIndex = 0;
		bool hasError = false;

		std::unordered_map<std::string, FunctionSignature> functionSignatures;

		StaticAnalysisVisitor(SymbolTable *table) {
			symbolTable = table;
		};
		~StaticAnalysisVisitor() = default;

		virtual std::any visitProg(ifccParser::ProgContext *ctx) override;
		virtual std::any visitFunction(ifccParser::FunctionContext *ctx) override;
		virtual std::any visitDeclareStatement(ifccParser::DeclareStatementContext *ctx) override;
		virtual std::any visitDeclareAssignStatement(ifccParser::DeclareAssignStatementContext *ctx) override;
		virtual std::any visitAssignStatement(ifccParser::AssignStatementContext *ctx) override;
		virtual std::any visitVarExpr(ifccParser::VarExprContext *ctx) override;
		virtual std::any visitFuncCallExpr(ifccParser::FuncCallExprContext *ctx) override;
		virtual std::any visitFunctionCallStatement(ifccParser::FunctionCallStatementContext *ctx) override;
};
