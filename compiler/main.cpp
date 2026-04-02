#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <unordered_map>
#include <string>

#include "antlr4-runtime.h"
#include "generated/ifccLexer.h"
#include "generated/ifccParser.h"
#include "generated/ifccBaseVisitor.h"

#include "SymbolTable.h"
#include "CodeGenVisitor.h"
#include "StaticAnalysisVisitor.h"
#include "IR.h"
#include "CFG.h"

using namespace antlr4;
using namespace std;

int main(int argn, const char **argv) {
	stringstream in;
	if (argn==2)
	{
		ifstream lecture(argv[1]);
		if( !lecture.good() )
		{
			cerr<<"error: cannot read file: " << argv[1] << endl ;
			exit(1);
		}
		in << lecture.rdbuf();
	}
	else
	{
		cerr << "usage: ifcc path/to/file.c" << endl ;
		exit(1);
	}

	ANTLRInputStream input(in.str());

	ifccLexer lexer(&input);
	CommonTokenStream tokens(&lexer);

	tokens.fill();

	ifccParser parser(&tokens);
	tree::ParseTree* tree = parser.axiom();

	if(parser.getNumberOfSyntaxErrors() != 0) {
		cerr << "error: syntax error during parsing" << endl;
		exit(1);
	}

	SymbolTable symbolTable;
	std::unordered_map<std::string, SymbolTable *> functionSymbolTables; // Map to hold symbol tables for each function
	std::unordered_map<std::string, FunctionSignature> functionSignatures; // Map to hold function signatures for call verification

	StaticAnalysisVisitor staticAnalysis(&symbolTable, &functionSymbolTables, &functionSignatures);
	staticAnalysis.visit(tree);

	if (staticAnalysis.hasError) {
		std::cerr << "Compilation failed due to static analysis errors." << std::endl;
		return 1;
	}

	CFGContainer cfgContainer(&functionSymbolTables);

	CodeGenVisitor codeGen(&cfgContainer, &functionSymbolTables, &functionSignatures);
	codeGen.visit(tree);

	cfgContainer.gen_asm(cout);

	return 0;
}
