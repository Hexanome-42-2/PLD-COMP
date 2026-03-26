#pragma once

#include "antlr4-runtime.h"
#include "generated/ifccBaseVisitor.h"
#include "SymbolTable.h"
#include <iostream>
#include <unordered_map>
#include <string>

// Function signature tracking for call verification
struct FunctionSignature {
	std::string returnType; // "int" or "void"
	int paramCount;
};

class StaticAnalysisVisitor : public ifccBaseVisitor {
	public:
		// Johny's per-function scope management
		SymbolTable *programSymbolTable;
		SymbolTable *currSymbolTable;
		std::unordered_map<std::string, SymbolTable*> *functionSymbolTables;

		int currIndex = 0;
		bool hasError = false;

		// Our function signature map for call verification
		std::unordered_map<std::string, FunctionSignature> functionSignatures;

		StaticAnalysisVisitor(SymbolTable *table, std::unordered_map<std::string, SymbolTable*> *funcTables)
			: programSymbolTable(table), functionSymbolTables(funcTables) {};
		~StaticAnalysisVisitor() = default;

		virtual std::any visitProg(ifccParser::ProgContext *ctx) override;
		virtual std::any visitFunction(ifccParser::FunctionContext *ctx) override;
		virtual std::any visitDeclareStatement(ifccParser::DeclareStatementContext *ctx) override;
		virtual std::any visitDeclareAssignStatement(ifccParser::DeclareAssignStatementContext *ctx) override;
		virtual std::any visitAssignStatement(ifccParser::AssignStatementContext *ctx) override;
		virtual std::any visitVarExpr(ifccParser::VarExprContext *ctx) override;
		virtual std::any visitFuncCall(ifccParser::FuncCallContext *ctx) override;
		virtual std::any visitFunctionCallStatement(ifccParser::FunctionCallStatementContext *ctx) override;
};
