#pragma once


#include "antlr4-runtime.h"
#include "generated/ifccBaseVisitor.h"
#include "StaticAnalysisVisitor.h"
#include "SymbolTable.h"
#include "IR.h"
#include "CFG.h"
#include <map>
#include <string>
#include <unordered_map>

class  CodeGenVisitor : public ifccBaseVisitor {
	public:
		CFG *currentCFG = nullptr; // Current CFG being accessed
		CFGContainer *cfgContainer;
		std::unordered_map<std::string, SymbolTable*> *functionSymbolTables;

		CodeGenVisitor(
			CFGContainer *cfgContainer,
			std::unordered_map<std::string, SymbolTable*> *functionSymbolTables
		) : cfgContainer(cfgContainer),
			functionSymbolTables(functionSymbolTables) {};

		virtual antlrcpp::Any visitProg(ifccParser::ProgContext *ctx) override;
		virtual antlrcpp::Any visitFunction(ifccParser::FunctionContext *ctx) override;
		virtual antlrcpp::Any visitBlock(ifccParser::BlockContext *ctx) override;
		virtual antlrcpp::Any visitDeclareStatement(ifccParser::DeclareStatementContext *ctx) override;
		virtual antlrcpp::Any visitDeclareAssignStatement(ifccParser::DeclareAssignStatementContext *ctx) override;
		virtual antlrcpp::Any visitAssignStatement(ifccParser::AssignStatementContext *ctx) override;
		virtual antlrcpp::Any visitFunctionCallStatement(ifccParser::FunctionCallStatementContext *ctx) override;
		virtual antlrcpp::Any visitReturnStatement(ifccParser::ReturnStatementContext *ctx) override;
		virtual antlrcpp::Any visitIfStatement(ifccParser::IfStatementContext *ctx) override;
		// virtual antlrcpp::Any visitWhileStatement(ifccParser::WhileStatementContext *ctx) override;

		// Expressions
		virtual antlrcpp::Any visitUnaryExpr(ifccParser::UnaryExprContext *ctx) override;
		virtual antlrcpp::Any visitFuncCall(ifccParser::FuncCallContext *ctx) override;
		virtual antlrcpp::Any visitConstExpr(ifccParser::ConstExprContext *ctx) override;
		virtual antlrcpp::Any visitVarExpr(ifccParser::VarExprContext *ctx) override;
		virtual antlrcpp::Any visitAddSub(ifccParser::AddSubContext *ctx) override;
		virtual antlrcpp::Any visitMultDiv(ifccParser::MultDivContext *ctx) override;
		virtual antlrcpp::Any visitBitWise(ifccParser::BitWiseContext *ctx) override;
		virtual antlrcpp::Any visitPar(ifccParser::ParContext *ctx) override;
		virtual antlrcpp::Any visitComp(ifccParser::CompContext *ctx) override;
		virtual antlrcpp::Any visitEQ(ifccParser::EQContext *ctx) override;
};

