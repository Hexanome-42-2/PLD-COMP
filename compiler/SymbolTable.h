#pragma once
#include <unordered_map>
#include <string>
#include <vector>
#include <iostream>

// Structure to store variable metadata
struct VarInfo {
	int index;	// Memory offset (multiple of 4)
	bool used;	// Whether the variable is read in an expression
};

class SymbolTable {
    private:
        std::unordered_map<std::string, VarInfo> symbolTable;
        int varOffset = 0; // Start at 0 and decrement by 4 for each new variable
        int tmpOffset = 0; // Start at 0 and increment by 4 for each new temporary variable
        //int maxOffset = 0;
    public:
        SymbolTable();
        ~SymbolTable();
        void addVariable(const std::string& name);
        const std::string addTemporaryVariable();
        VarInfo* getVariable(const std::string& name);
        int getVariableOffset(const std::string& name);
        bool getUsed(const std::string& name);
        void MarkUsed(const std::string& name);
        void InitializeTmpOffset();
        std::vector<std::string> getUnusedVariables();
        void printSymbolTable() const; // Optional: For debugging purposes
};