#pragma once

#include "antlr4-runtime.h"
#include "generated/ifccBaseVisitor.h"
#include "SymbolTable.h"
#include <iostream>
#include <unordered_map>
#include <string>

class StaticAnalysisVisitor : public ifccBaseVisitor {
	public:
		SymbolTable *programSymbolTable;  // Symbol table for the entire program (global scope)
		SymbolTable *currSymbolTable; // Pointer to the current symbol table being accessed (either global or function scope)
		std::unordered_map<std::string, SymbolTable*> *functionSymbolTables;  // Map to hold symbol tables for each function

		int currIndex = 0;
		bool hasError = false;

		StaticAnalysisVisitor(SymbolTable *table, std::unordered_map<std::string, SymbolTable*> *funcTables) : programSymbolTable(table), functionSymbolTables(funcTables) {
		};
		~StaticAnalysisVisitor() = default;

		virtual std::any visitProg(ifccParser::ProgContext *ctx) override;
		virtual std::any visitFunction(ifccParser::FunctionContext *ctx) override;
		virtual std::any visitDeclareStatement(ifccParser::DeclareStatementContext *ctx) override;
		virtual std::any visitAssignStatement(ifccParser::AssignStatementContext *ctx) override;
		virtual std::any visitVarExpr(ifccParser::VarExprContext *ctx) override;
};
