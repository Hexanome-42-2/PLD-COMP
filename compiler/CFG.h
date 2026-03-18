#pragma once

#include "IR.h"
#include "SymbolTable.h"
#include<unordered_map>
#include<string>

class BasicBlock;

// Class for the Control Flow Graph (CFG)
class CFG {
 public:
	//CFG(DefFonction* ast);
    CFG(SymbolTable * aSymbolTable, std::string aName) : symbolTable(aSymbolTable), name(aName), nextBBnumber(0), bbs(std::vector <BasicBlock*>()), current_bb(nullptr) {}

	~CFG();

	//DefFonction* ast; /**< The AST this CFG comes from */
	
	void add_bb(BasicBlock* bb); //

	// x86 code generation: could be encapsulated in a processor class in a retargetable compiler
	std::string IR_reg_to_asm(std::string reg); /**< helper method: inputs a IR reg or input variable, returns e.g. "-24(%rbp)" for the proper value of 24 */
	virtual void gen_asm(std::ostream& o);
	virtual void gen_asm_prologue(std::ostream& o);
	virtual void gen_asm_epilogue(std::ostream& o);

	// symbol table methods
	void add_to_symbol_table(std::string name, Type t);
	std::string create_new_tempvar(Type t);
	int get_var_index(std::string name);
	Type get_var_type(std::string name);

	// basic block management
	std::string new_BB_name(); //
	BasicBlock* current_bb;

 protected:
	SymbolTable * symbolTable; /**< part of the symbol table  */
	int nextBBnumber; /**< just for naming */
	
	std::vector <BasicBlock*> bbs; /**< all the basic blocks of this CFG*/
	std::string name; /**< the name of the function this CFG represents (e.g., "main" for the main CFG) */
};

class CFGContainer : CFG {
    private:
        std::unordered_map <std::string, CFG*> cfgs;
		std::unordered_map <std::string, SymbolTable*> *symbolTables;
    public:
		CFGContainer(std::unordered_map <std::string, SymbolTable*> *symbolTables) : symbolTables(symbolTables), CFG(nullptr, "") {};
		~CFGContainer();
		void add_cfg(std::string name, CFG* cfg);
		CFG* get_cfg(std::string name);

		virtual void gen_asm(std::ostream &o);
		virtual void gen_asm_prologue(std::ostream &o);
		virtual void gen_asm_epilogue(std::ostream &o);
};
