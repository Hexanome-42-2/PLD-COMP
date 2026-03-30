#pragma once

#include "antlr4-runtime.h"
#include "generated/ifccBaseVisitor.h"
#include "SymbolTable.h"
#include <iostream>
#include <unordered_map>
#include <string>

// Function signature tracking for call verification
struct FunctionSignature {
	Type returnType;
	int paramCount;
	bool isExternal; // To add dynamic library support
};

class StaticAnalysisVisitor : public ifccBaseVisitor {
	public:
		// Johny's per-function scope management
		SymbolTable *programSymbolTable;
		SymbolTable *currSymbolTable;
		std::unordered_map<std::string, SymbolTable*> *allSymbolTables;

		int currIndex = 0;
		bool hasError = false;
		std::string currentFunctionName;
        bool isVisitingInclude = false; // Flag to track if we're currently visiting an include statement

		// Our function signature map for call verification
		std::unordered_map<std::string, FunctionSignature> functionSignatures;

		StaticAnalysisVisitor(SymbolTable *table, std::unordered_map<std::string, SymbolTable*> *funcTables)
			: programSymbolTable(table), allSymbolTables(funcTables) {};
		~StaticAnalysisVisitor() = default;

		virtual std::any visitIncludeStatement(ifccParser::IncludeStatementContext *ctx) override;
		virtual std::any visitProg(ifccParser::ProgContext *ctx) override;
		virtual std::any visitFunctionDeclaration(ifccParser::FunctionDeclarationContext *ctx) override;
		virtual std::any visitFunctionDefinition(ifccParser::FunctionDefinitionContext *ctx) override;
        virtual std::any visitBlock(ifccParser::BlockContext *ctx) override;
        virtual std::any visitDeclareStatement(ifccParser::DeclareStatementContext *ctx) override;
		virtual std::any visitAssignStatement(ifccParser::AssignStatementContext *ctx) override;
		virtual std::any visitVarExpr(ifccParser::VarExprContext *ctx) override;
		virtual std::any visitFuncCall(ifccParser::FuncCallContext *ctx) override;
		virtual std::any visitFunctionCallStatement(ifccParser::FunctionCallStatementContext *ctx) override;
};
