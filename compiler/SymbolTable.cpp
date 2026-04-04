#include "SymbolTable.h"

Type stringToType(const std::string& typeStr) {
    if (typeStr == "int") {
        return Type::INT;
    } else if (typeStr == "void") {
        return Type::VOID;
    } else {
        return Type::ERROR; // Invalid type
    }
}

int typeSizeOf(Type type) {
    switch (type) {
        case Type::INT:
            return 4;
        case Type::VOID:
            return 0; // Void has no size
        default:
            return 0;
    }
}

SymbolTable::~SymbolTable() {

}

void SymbolTable::addVariable(const std::string &name) {
    varOffset -= 4; // Decrement offset for the next variable
    symbolTable[name] = {varOffset, Type::INT, false}; // Store the current offset and mark as unused

    // Ensure frame size accounts for regular variables (including params).
    if (varOffset < maxOffset) {
        maxOffset = varOffset;
    }
}

const std::string SymbolTable::addTemporaryVariable() {
    tmpOffset -= 4; // Decrement offset for the next temporary variable
    std::string tmpName = nameTemporaryVariable(tmpOffset); // Create a unique name for the temporary variable
    symbolTable[tmpName] = {tmpOffset, Type::INT, false}; // Store the current offset and mark as unused
    return tmpName; // Return the name of the temporary variable
}

std::string SymbolTable::nameTemporaryVariable(int offset) {
    return "!tmp" + std::to_string(offset);
}

VarInfo *SymbolTable::getVariable(const std::string &name) {
    auto it = symbolTable.find(name);
    if (it != symbolTable.end()) {
        return &(it->second); // Return a pointer to the VarInfo
    }
    if (parent != nullptr) {
        return parent->getVariable(name); // Variable not found, check parent
    }
    return nullptr; // Variable not found
}

VarInfo* SymbolTable::getLocalVariable(const std::string& name) {
    auto it = symbolTable.find(name);
    if (it != symbolTable.end()) {
        return &(it->second);
    }
    return nullptr; 
}

int SymbolTable::getVariableOffset(const std::string &name) {
    auto it = symbolTable.find(name);
    if (it != symbolTable.end()) {
        return it->second.index;
    }
    if (parent != nullptr) {
        return parent->getVariableOffset(name); // Variable not found, check parent
    }
    return 1; // Variable not found
}

Type SymbolTable::getVariableType(const std::string &name) {
    auto it = symbolTable.find(name);
    if (it != symbolTable.end()) {
        return it->second.type;
    }
    if (parent != nullptr) {
        return parent->getVariableType(name); // Variable not found, check parent
    }
    return Type::ERROR; // Variable not found
}

void SymbolTable::clearTemporaryVariables() {
    tmpOffset = varOffset;
}

int SymbolTable::getMaxOffset() const {
    return maxOffset;
}

bool SymbolTable::getUsed(const std::string &name) {
    auto it = symbolTable.find(name);
    if (it != symbolTable.end()) {
        return it->second.used;
    }
    if (parent != nullptr) {
        return parent->getUsed(name); // Variable not found, check parent
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
    if (varOffset < maxOffset) maxOffset = varOffset;
    if (tmpOffset < maxOffset) maxOffset = tmpOffset;
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
    if (parent != nullptr) {
        parent->printSymbolTable();
        std::cout << "New scope:\n";
    } else {
        std::cout << "Symbol Table:\n";
    }
    for (const auto& pair : symbolTable) {
        std::cout << "Variable: " << pair.first << ", Offset: " << pair.second.index << ", Used: " << pair.second.used << "\n";
    }
}

void SymbolTable::updateStackSize() {
    stackSize -= maxOffset;
    if (parent != nullptr) {
        // Update the parent's stack size if this scope requires more space
        parent->stackSize = std::max(parent->stackSize, stackSize);
    }
}