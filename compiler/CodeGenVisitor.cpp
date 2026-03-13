#include "CodeGenVisitor.h"

antlrcpp::Any CodeGenVisitor::visitProg(ifccParser::ProgContext *ctx) {
	cfg->add_bb(new BasicBlock(cfg, cfg->new_BB_name()));
    visitChildren(ctx);
    return 0;
}

antlrcpp::Any CodeGenVisitor::visitDeclareStatement(ifccParser::DeclareStatementContext *ctx) {    
    // ~~~~~~ CAN'T ASSIGN AND DECLARE ATM ~~~~~~
	return 0;
}

antlrcpp::Any CodeGenVisitor::visitAssignStatement(ifccParser::AssignStatementContext *ctx) {
	// Visit the expression, evaluating the right side and putting the result in %eax
	visit(ctx->expr());
	
	std::string varName = ctx->VAR()->getText();

	cfg->current_bb->add_IRInstr(IRInstr::Operation::wmem, Type::INT, {varName, "eax"});

	return 0;
}

antlrcpp::Any CodeGenVisitor::visitReturnStatement(ifccParser::ReturnStatementContext *ctx) {
	// Evaluate the result of the expression
	visit(ctx->expr());

	return 0;
}

// ~~~~~~~~ Expressions ~~~~~~~~

antlrcpp::Any CodeGenVisitor::visitUnaryExpr(ifccParser::UnaryExprContext *ctx) {
	visit(ctx->expr_unary());
	if (ctx->NEG()) {
    	cfg->current_bb->add_IRInstr(IRInstr::Operation::negl, Type::INT, {});
	}
	return 0;
}

antlrcpp::Any CodeGenVisitor::visitConstExpr(ifccParser::ConstExprContext *ctx) {
	std::string constValue = ctx->CONST()->getText();

	cfg->current_bb->add_IRInstr(IRInstr::Operation::ldconst, Type::INT, {"eax", constValue});

    return 0;
}

antlrcpp::Any CodeGenVisitor::visitVarExpr(ifccParser::VarExprContext *ctx) {
	std::string varName = ctx->VAR()->getText();

	cfg->current_bb->add_IRInstr(IRInstr::Operation::rmem, Type::INT, {"eax", varName});

	return 0;
}

antlrcpp::Any CodeGenVisitor::visitAddSub(ifccParser::AddSubContext *ctx) {
	visit(ctx->lExpr);
	const std::string tmpVar = cfg->create_new_tempvar(Type::INT);
	cfg->current_bb->add_IRInstr(IRInstr::Operation::wmem, Type::INT, {tmpVar, "eax"});

	visit(ctx->rExpr);
	cfg->current_bb->add_IRInstr(IRInstr::Operation::rmem, Type::INT, {"edx", tmpVar});
	cfg->current_bb->add_IRInstr(IRInstr::Operation::add, Type::INT, {"eax", "edx"});
	return 0;
}

antlrcpp::Any CodeGenVisitor::visitMultDiv(ifccParser::MultDivContext *ctx) {
	visit(ctx->lExpr);	
	const std::string tmpVar = cfg->create_new_tempvar(Type::INT);
	cfg->current_bb->add_IRInstr(IRInstr::Operation::wmem, Type::INT, {tmpVar, "eax"});

	visit(ctx->rExpr);
	cfg->current_bb->add_IRInstr(IRInstr::Operation::rmem, Type::INT, {"edx", tmpVar});
	cfg->current_bb->add_IRInstr(IRInstr::Operation::mul, Type::INT, {"eax", "edx"});
	return 0;
}

antlrcpp::Any CodeGenVisitor::visitPar(ifccParser::ParContext *ctx) {
	visit(ctx->expr());
	return 0;
}
