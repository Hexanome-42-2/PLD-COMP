#pragma once
#include <unordered_map>
#include <string>
#include <vector>
#include <iostream>

enum Type {INT, VOID, ERROR};

Type stringToType(const std::string& typeStr);
int typeSizeOf(Type type);

// Structure to store variable metadata
struct VarInfo {
	int index;	// Memory offset (multiple of 4)
    Type type;
	bool used;	// Whether the variable is read in an expression
};

class SymbolTable {
    private:
        SymbolTable* parent; // Pointer to the parent symbol table (for nested scopes)
        std::string stName; // SymbolTableName
        int stackSize = 0; // Total size of the stack frame for this scope
        std::unordered_map<std::string, VarInfo> symbolTable;
        int varOffset = 0; // Start at 0 and decrement by 4 for each new variable
        int tmpOffset = 0; // Start at 0 and increment by 4 for each new temporary variable
        int maxOffset = 0;
    public:
        SymbolTable(SymbolTable* parent = nullptr, std::string name = "global") 
            : parent(parent), stName(name) {
            if (parent != nullptr) {
                varOffset = parent->varOffset;
                tmpOffset = varOffset;
                maxOffset = varOffset;
            }
        };
        ~SymbolTable();
        std::string getName() const { return stName; }
        void updateStackSize(); // Once the scope is processed, add stacksize padding and update the parent's max stackSize (amongst siblings)
        int getStackSize() const { return stackSize; }
        void addVariable(const std::string& name);
        const std::string addTemporaryVariable();
        std::string nameTemporaryVariable(int offset);
        VarInfo* getVariable(const std::string& name);
        VarInfo* getLocalVariable(const std::string& name);
        int getVariableOffset(const std::string& name);
        Type getVariableType(const std::string& name);
        void clearTemporaryVariables();
        int getMaxOffset() const;
        bool getUsed(const std::string& name);
        void MarkUsed(const std::string& name);
        int getVarOffset() const { return varOffset; }
        void setVarOffset(int offset) { varOffset = offset; }
        void InitializeTmpOffset();
        void updateMaxOffset();
        std::vector<std::string> getUnusedVariables();
        void printSymbolTable() const; // Optional: For debugging purposes
};

