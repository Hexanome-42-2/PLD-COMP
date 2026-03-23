#include "CodeGenVisitor.h"

antlrcpp::Any CodeGenVisitor::visitProg(ifccParser::ProgContext *ctx) {
    visitChildren(ctx);
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitFunction(ifccParser::FunctionContext *ctx) {
	// 1. Create a new CFG for this function and add it to the map
	std::string functionName = ctx->funcName->getText();
	CFG* oldCFG = currentCFG; // Save the current CFG to restore it later
	currentCFG = new CFG((*functionSymbolTables)[functionName], functionName);

	cfgContainer->add_cfg(functionName, currentCFG);

	currentCFG->add_bb(new BasicBlock(currentCFG, currentCFG->new_BB_name())); // Start with a new basic block for the function entry

	// 2. Visit the function block to generate IR
	visit(ctx->block());
	
	// 3. Restore locations
	currentCFG = oldCFG; // Restore the previous CFG

	return 0;
}

antlrcpp::Any CodeGenVisitor::visitDeclareStatement(ifccParser::DeclareStatementContext *ctx) {
    // Declaration only, no code to generate
	return 0;
}

antlrcpp::Any CodeGenVisitor::visitDeclareAssignStatement(ifccParser::DeclareAssignStatementContext *ctx) {
	// Evaluate the expression on the right side
	visit(ctx->expr());

	// Store result into the newly declared variable
	std::string varName = ctx->NAME()->getText();
	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::wmem, Type::INT, {varName, "eax"});

	return 0;
}

antlrcpp::Any CodeGenVisitor::visitFunctionCallStatement(ifccParser::FunctionCallStatementContext *ctx) {
	// TODO: push arguments to registers per calling convention
	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::call, Type::INT, {ctx->NAME()->getText()});
	return 0;
}

antlrcpp::Any CodeGenVisitor::visitAssignStatement(ifccParser::AssignStatementContext *ctx) {
	// Visit the expression, evaluating the right side and putting the result in %eax
	visit(ctx->expr());
	
	std::string varName = ctx->NAME()->getText();

	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::wmem, Type::INT, {varName, "eax"});

	return 0;
}

antlrcpp::Any CodeGenVisitor::visitReturnStatement(ifccParser::ReturnStatementContext *ctx) {
	// Evaluate the result of the expression (if present — void functions have no return expr)
	if (ctx->expr()) {
		visit(ctx->expr());
	}

	return 0;
}

// ~~~~~~~~ Expressions ~~~~~~~~

antlrcpp::Any CodeGenVisitor::visitUnaryExpr(ifccParser::UnaryExprContext *ctx) {
	visit(ctx->expr_unary());
	if (ctx->NEG()) {
    	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::negl, Type::INT, {});
	}
	return 0;
}

antlrcpp::Any CodeGenVisitor::visitFuncCall(ifccParser::FuncCallContext *ctx) {
	// Implementation for function call generation
	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::call, Type::INT, {ctx->NAME()->getText()});	
	return 0;
}

antlrcpp::Any CodeGenVisitor::visitConstExpr(ifccParser::ConstExprContext *ctx) {
	std::string constValue = ctx->CONST()->getText();

	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::ldconst, Type::INT, {"eax", constValue});

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitVarExpr(ifccParser::VarExprContext *ctx) {
	std::string varName = ctx->NAME()->getText();

	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::rmem, Type::INT, {"eax", varName});

	return 0;
}

antlrcpp::Any CodeGenVisitor::visitAddSub(ifccParser::AddSubContext *ctx) {
	visit(ctx->lExpr);
	const std::string tmpVar = currentCFG->create_new_tempvar(Type::INT);
	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::wmem, Type::INT, {tmpVar, "eax"});

	visit(ctx->rExpr);
	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::rmem, Type::INT, {"edx", tmpVar});
	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::add, Type::INT, {"eax", "edx"});
	return 0;
}

antlrcpp::Any CodeGenVisitor::visitMultDiv(ifccParser::MultDivContext *ctx) {
	visit(ctx->lExpr);	
	const std::string tmpVar = currentCFG->create_new_tempvar(Type::INT);
	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::wmem, Type::INT, {tmpVar, "eax"});

	visit(ctx->rExpr);
	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::rmem, Type::INT, {"edx", tmpVar});
	currentCFG->current_bb->add_IRInstr(IRInstr::Operation::mul, Type::INT, {"eax", "edx"});
	return 0;
}

antlrcpp::Any CodeGenVisitor::visitPar(ifccParser::ParContext *ctx) {
	visit(ctx->expr());
	return 0;
}
