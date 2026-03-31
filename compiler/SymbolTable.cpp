#include "SymbolTable.h"

SymbolTable::SymbolTable() {
}

SymbolTable::~SymbolTable() {

}

void SymbolTable::addVariable(const std::string &name) {
    varOffset -= 4; // Decrement offset for the next variable
    symbolTable[name] = {varOffset, Type::INT, false}; // Store the current offset and mark as unused
}

const std::string SymbolTable::addTemporaryVariable() {
    tmpOffset -= 4; // Decrement offset for the next temporary variable
    std::string tmpName = "!tmp" + std::to_string(tmpOffset); // Create a unique name for the temporary variable
    symbolTable[tmpName] = {tmpOffset, Type::INT, false}; // Store the current offset and mark as unused
    return tmpName; // Return the name of the temporary variable
}

VarInfo *SymbolTable::getVariable(const std::string &name) {
    auto it = symbolTable.find(name);
    if (it != symbolTable.end()) {
        return &(it->second); // Return a pointer to the VarInfo
    }
    return nullptr; // Variable not found
}

int SymbolTable::getVariableOffset(const std::string &name) {
    auto it = symbolTable.find(name);
    if (it != symbolTable.end()) {
        return it->second.index;
    }
    return 1; // Variable not found
}

Type SymbolTable::getVariableType(const std::string &name) {
    auto it = symbolTable.find(name);
    if (it != symbolTable.end()) {
        return it->second.type;
    }
    return Type::ERROR; // Variable not found
}

int SymbolTable::getMaxOffset() const {
    return maxOffset;
}

bool SymbolTable::getUsed(const std::string &name) {
    auto it = symbolTable.find(name);
    if (it != symbolTable.end()) {
        return it->second.used;
    }
    return false; // Variable not found
}

void SymbolTable::MarkUsed(const std::string &name) {
    auto it = symbolTable.find(name);
    if (it != symbolTable.end()) {
        it->second.used = true; // Mark the variable as used
    }
}

void SymbolTable::InitializeTmpOffset() {
    tmpOffset = varOffset; // Start temporary variable offsets from the current variable offset
}

void SymbolTable::updateMaxOffset() {
    maxOffset = tmpOffset < maxOffset ? tmpOffset : maxOffset;
}

std::vector<std::string> SymbolTable::getUnusedVariables() {
    std::vector<std::string> unusedVars;
    for (const auto& pair : symbolTable) {
        if (!pair.second.used) {
            unusedVars.push_back(pair.first); // Add the name of the unused variable to the list
        }
    }
    return unusedVars;
}

void SymbolTable::printSymbolTable() const {
    std::cout << "Symbol Table:\n";
    for (const auto& pair : symbolTable) {
        std::cout << "Variable: " << pair.first << ", Offset: " << pair.second.index << ", Used: " << pair.second.used << "\n";
    }
}