#ifndef __CODE_GENERATOR_HPP__
#define __CODE_GENERATOR_HPP__

#include "Instruction.hpp"
#include "../Semantic/AST.hpp"
#include "../Semantic/SymbolTable.hpp"
#include <map>
#include <ostream>
#include <string>
#include <vector>

class CodeGenerator {
private:
    const SymbolTable *symbolTable;
    std::vector<Instruction> instructions;
    std::vector<std::string> errors;
    std::map<int, int> subprogramEntries;
    std::map<std::string, int> subprogramEntriesByName;
    std::map<int, int> subprogramParamCounts;
    std::map<std::string, int> subprogramParamCountsByName;
    std::map<int, int> subprogramParamSlots;
    std::map<std::string, int> subprogramParamSlotsByName;
    int currentFrameLevel;

    struct RuntimeLocation {
        int levelDiff;
        int address;

        RuntimeLocation(int l = -1, int a = -1) : levelDiff(l), address(a) {}
        bool isValid() const { return levelDiff >= 0 && address >= 0; }
    };

    int emit(const std::string &op, int level, const std::string &argument);
    int emit(const std::string &op, int level, int argument);
    void patchArgument(int instructionIndex, int targetLine);

    void generateNode(ASTNode *node);
    void generateStatement(ASTNode *node);
    void generateExpression(ASTNode *node);

    void generateProgram(ProgramNode *node);
    void generateBlock(BlockNode *node);
    void generateDeclarationList(DeclarationListNode *node);
    void generateAssignment(AssignNode *node);
    void generateProcedureCall(ProcCallNode *node);
    void generateIf(IfNode *node);
    void generateWhile(WhileNode *node);
    void generateRepeat(RepeatNode *node);
    void generateFor(ForNode *node);
    void generateCase(CaseNode *node);
    void generateSubprogramDeclaration(SubprogramDeclNode *node);

    int currentLine() const;
    int globalFrameSize() const;
    int frameSizeForBlock(const ASTNode *node) const;
    RuntimeLocation runtimeLocation(const ASTNode *node) const;
    RuntimeLocation runtimeLocationByName(const std::string &name) const;
    bool generateAddress(ASTNode *node);
    bool storeTopToTarget(ASTNode *node);
    bool copyStructuredValue(ASTNode *source, ASTNode *target, int size);
    bool generateLoadSlots(ASTNode *node, int size);
    bool generateStoreInput(ASTNode *node);
    bool isStructured(const ASTNode *node) const;
    int storageSize(const ASTNode *node) const;
    int operationForBinary(const std::string &op) const;
    void generateSubprogramCall(const std::string &name, int tabRef, const std::vector<ASTNode *> &args, bool leavesResult);
    int subprogramEntry(int tabRef, const std::string &name) const;
    int expectedParamCount(int tabRef, const std::string &name) const;
    int expectedParamSlotCount(int tabRef, const std::string &name) const;
    std::string formatCallArgument(int entry, int paramCount) const;
    std::string formatReal(double value) const;
    std::string quoteLiteral(const std::string &value) const;
    std::string lower(const std::string &value) const;
    void addError(const std::string &message);

public:
    CodeGenerator();

    const std::vector<Instruction> &generate(ASTNode *ast, const SymbolTable &table);

    const std::vector<Instruction> &getInstructions() const { return instructions; }
    const std::vector<std::string> &getErrors() const { return errors; }
    bool hasErrors() const { return !errors.empty(); }

    void print(std::ostream &out) const;
};

#endif
