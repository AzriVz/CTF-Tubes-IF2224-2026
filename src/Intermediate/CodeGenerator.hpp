#ifndef __CODE_GENERATOR_HPP__
#define __CODE_GENERATOR_HPP__

#include "Instruction.hpp"
#include "../Semantic/AST.hpp"
#include "../Semantic/SymbolTable.hpp"
#include <ostream>
#include <string>
#include <vector>

class CodeGenerator {
private:
    const SymbolTable *symbolTable;
    std::vector<Instruction> instructions;
    std::vector<std::string> errors;

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

    int currentLine() const;
    int globalFrameSize() const;
    int runtimeAddress(const ASTNode *node) const;
    int runtimeAddressByName(const std::string &name) const;
    int operationForBinary(const std::string &op) const;
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
