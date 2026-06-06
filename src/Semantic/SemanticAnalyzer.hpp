#ifndef __SEMANTIC_ANALYZER_HPP__
#define __SEMANTIC_ANALYZER_HPP__

#include "../Parser/Parser.hpp"
#include "AST.hpp"
#include "SymbolTable.hpp"
#include <map>
#include <string>
#include <vector>

class SemanticAnalyzer
{
private:
    SymbolTable symTable;
    std::vector<std::string> errors;
    std::vector<std::string> warnings;
    std::map<std::string, ASTNode *> namedTypeNodes;
    std::map<std::string, int> namedTypeSizes;
    std::map<int, ASTNode *> typeNodeBySymbol;
    std::map<int, int> sizeBySymbol;

    int currentLine;

    ParseNode *findChild(ParseNode *node, const std::string &name) const;

    std::vector<ParseNode *> findAllChildren(ParseNode *node, const std::string &name) const;

    ParseNode *getChild(ParseNode *node, int index) const;

    std::string getToken(ParseNode *node) const;

    bool hasChild(ParseNode *node, const std::string &name) const;

    void addError(const std::string &msg);
    void addWarning(const std::string &msg);

    bool isCompatible(int typeCode1, int typeCode2) const;

    bool isAssignmentCompatible(int targetType, int valueType) const;

    bool isNumeric(int typeCode) const;

    bool isBoolean(int typeCode) const;

    bool isOrdinal(int typeCode) const;

    int inferBinOpType(const std::string &op, int leftType, int rightType, int line);

    int inferUnaryOpType(const std::string &op, int operandType, int line);

    std::string typeCodeToString(int typeCode) const;
    ASTNode *resolveTypeNode(const std::string &typeName, ASTNode *typeNode) const;
    int typeSize(const std::string &typeName, ASTNode *typeNode) const;
    int typeRef(ASTNode *typeNode) const;
    int ordinalValue(ASTNode *node, bool &ok) const;
    ArrayTypeNode *arrayTypeFor(ASTNode *node) const;
    RecordTypeNode *recordTypeFor(ASTNode *node) const;

    ASTNode *visit(ParseNode *node);

    ASTNode *visitProgram(ParseNode *node);
    std::string visitProgramHeader(ParseNode *node);

    ASTNode *visitDeclarationPart(ParseNode *node);
    ASTNode *visitConstDeclaration(ParseNode *node);
    ASTNode *visitConstant(ParseNode *node);
    ASTNode *visitTypeDeclaration(ParseNode *node);
    ASTNode *visitVarDeclaration(ParseNode *node);
    std::vector<std::string> visitIdentifierList(ParseNode *node);

    ASTNode *visitCompoundStatement(ParseNode *node);
    ASTNode *visitStatementList(ParseNode *node);
    ASTNode *visitAssignmentStatement(ParseNode *node);
    ASTNode *visitProcedureCall(ParseNode *node);
    ASTNode *visitIfStatement(ParseNode *node);
    ASTNode *visitWhileStatement(ParseNode *node);
    ASTNode *visitForStatement(ParseNode *node);
    ASTNode *visitRepeatStatement(ParseNode *node);
    ASTNode *visitCaseStatement(ParseNode *node);
    ASTNode *visitCaseBlock(ParseNode *node);

    ASTNode *visitExpression(ParseNode *node);
    ASTNode *visitSimpleExpression(ParseNode *node);
    ASTNode *visitTerm(ParseNode *node);
    ASTNode *visitFactor(ParseNode *node);
    ASTNode *visitVariable(ParseNode *node);
    ASTNode *visitComponentVariable(ParseNode *node, ASTNode *base);

    ASTNode *visitType(ParseNode *node, std::string &outTypeName);
    ASTNode *visitArrayType(ParseNode *node, std::string &outTypeName);
    ASTNode *visitRecordType(ParseNode *node, std::string &outTypeName);
    ASTNode *visitRange(ParseNode *node, std::string &outTypeName);
    ASTNode *visitEnumerated(ParseNode *node, std::string &outTypeName);

    ASTNode *visitSubprogramDeclaration(ParseNode *node);
    ASTNode *visitProcedureDeclaration(ParseNode *node);
    ASTNode *visitFunctionDeclaration(ParseNode *node);
    ASTNode *visitFormalParameterList(ParseNode *node, std::vector<VarDeclNode *> &params);
    ASTNode *visitParameterGroup(ParseNode *node, std::vector<VarDeclNode *> &params);

    std::string visitRelationalOperator(ParseNode *node);
    std::string visitAdditiveOperator(ParseNode *node);
    std::string visitMultiplicativeOperator(ParseNode *node);

public:
    SemanticAnalyzer();

    ASTNode *analyze(ParseNode *parseTree);

    bool hasErrors() const { return !errors.empty(); }

    const std::vector<std::string> &getErrors() const { return errors; }

    const std::vector<std::string> &getWarnings() const { return warnings; }

    const SymbolTable &getSymbolTable() const { return symTable; }

    void printResults(std::ostream &out, ASTNode *ast);
};

#endif
