#ifndef __AST_HPP__
#define __AST_HPP__

#include <string>
#include <vector>
#include <iostream>
#include <sstream>

class ASTNode;

class ASTNode {
public:
    std::string type;
    int level;
    int tabRef;
    int line;
    int storageSize;

    std::vector<ASTNode *> children;

    ASTNode();
    virtual ~ASTNode();

    virtual std::string nodeName() const = 0;

    virtual std::string annotation() const;

    virtual std::string label() const;

    void addChild(ASTNode *child);

    void print(std::ostream &out, const std::string &prefix = "", bool isLast = true) const;
};

class ProgramNode : public ASTNode {
public:
    std::string name;
    ASTNode *declarations;
    ASTNode *body;

    ProgramNode(const std::string &n, ASTNode *decls, ASTNode *blk);
    std::string nodeName() const override { return "ProgramNode"; }
    std::string label() const override;
    std::string annotation() const override;
};

class BlockNode : public ASTNode {
public:
    int blockIndex;

    BlockNode(ASTNode *stmts = nullptr);
    std::string nodeName() const override { return "BlockNode"; }
    std::string annotation() const override;
};

class DeclarationListNode : public ASTNode {
public:
    DeclarationListNode();
    std::string nodeName() const override { return "DeclarationListNode"; }

    void addDeclaration(ASTNode *decl);
};

class VarDeclNode : public ASTNode {
public:
    std::string varName;
    std::string typeName;
    ASTNode *typeNode;
    int offset;
    int size;

    VarDeclNode(const std::string &name, const std::string &tname, ASTNode *tnode = nullptr);
    std::string nodeName() const override { return "VarDecl"; }
    std::string label() const override;
    std::string annotation() const override;
};

class ConstDeclNode : public ASTNode {
public:
    std::string constName;
    ASTNode *value;

    ConstDeclNode(const std::string &name, ASTNode *val);
    std::string nodeName() const override { return "ConstDecl"; }
    std::string label() const override;
    std::string annotation() const override;
};

class TypeDeclNode : public ASTNode {
public:
    std::string typeName;
    ASTNode *typeDef;

    TypeDeclNode(const std::string &name, ASTNode *def);
    std::string nodeName() const override { return "TypeDecl"; }
    std::string label() const override;
    std::string annotation() const override;
};

class AssignNode : public ASTNode {
public:
    ASTNode *target;
    ASTNode *value;

    AssignNode(ASTNode *tgt, ASTNode *val);
    std::string nodeName() const override { return "Assign"; }
    std::string label() const override;
    std::string annotation() const override;
};

class ProcCallNode : public ASTNode {
public:
    std::string procName;
    std::vector<ASTNode *> args;

    ProcCallNode(const std::string &name, const std::vector<ASTNode *> &arguments);
    std::string nodeName() const override { return "ProcCall"; }
    std::string label() const override;
    std::string annotation() const override;
};

class IfNode : public ASTNode {
public:
    ASTNode *condition;
    ASTNode *thenStmt;
    ASTNode *elseStmt;

    IfNode(ASTNode *cond, ASTNode *thenS, ASTNode *elseS = nullptr);
    std::string nodeName() const override { return "IfNode"; }
    std::string annotation() const override;
};

class WhileNode : public ASTNode {
public:
    ASTNode *condition;
    ASTNode *body;

    WhileNode(ASTNode *cond, ASTNode *bdy);
    std::string nodeName() const override { return "WhileNode"; }
    std::string annotation() const override;
};

class ForNode : public ASTNode {
public:
    std::string loopVar;
    std::string direction;
    ASTNode *start;
    ASTNode *end;
    ASTNode *body;

    ForNode(const std::string &var, const std::string &dir,
            ASTNode *st, ASTNode *en, ASTNode *bdy);
    std::string nodeName() const override { return "ForNode"; }
    std::string label() const override;
    std::string annotation() const override;
};

class RepeatNode : public ASTNode {
public:
    ASTNode *body;
    ASTNode *condition;

    RepeatNode(ASTNode *bdy, ASTNode *cond);
    std::string nodeName() const override { return "RepeatNode"; }
    std::string annotation() const override;
};

class CaseBranchNode : public ASTNode {
public:
    std::vector<ASTNode *> labels;
    ASTNode *statement;

    CaseBranchNode(const std::vector<ASTNode *> &lbls, ASTNode *stmt);
    std::string nodeName() const override { return "CaseBranch"; }
};

class CaseNode : public ASTNode {
public:
    ASTNode *selector;
    std::vector<CaseBranchNode *> branches;

    CaseNode(ASTNode *sel, const std::vector<CaseBranchNode *> &brs);
    std::string nodeName() const override { return "CaseNode"; }
    std::string annotation() const override;
};

class EmptyNode : public ASTNode {
public:
    EmptyNode();
    std::string nodeName() const override { return "EmptyNode"; }
};

class BinOpNode : public ASTNode {
public:
    std::string op;

    ASTNode *left;
    ASTNode *right;

    BinOpNode(const std::string &oper, ASTNode *l, ASTNode *r);
    std::string nodeName() const override { return "BinOp"; }
    std::string label() const override;
    std::string annotation() const override;
};

class UnaryOpNode : public ASTNode {
public:
    std::string op;
    ASTNode *operand;

    UnaryOpNode(const std::string &oper, ASTNode *opnd);
    std::string nodeName() const override { return "UnaryOp"; }
    std::string label() const override;
    std::string annotation() const override;
};

class VarNode : public ASTNode {
public:
    std::string name;

    VarNode(const std::string &n);
    std::string nodeName() const override { return "Var"; }
    std::string label() const override;
    std::string annotation() const override;
};

class ArrayAccessNode : public ASTNode {
public:
    ASTNode *array;
    ASTNode *index;
    int lowBound;
    int highBound;
    int elementSize;
    int totalSize;
    std::string elementTypeName;
    ASTNode *elementTypeNode;

    ArrayAccessNode(ASTNode *arr, ASTNode *idx);
    std::string nodeName() const override { return "ArrayAccess"; }
    std::string annotation() const override;
};

class RecordAccessNode : public ASTNode {
public:
    ASTNode *record;
    std::string field;
    int fieldOffset;
    int fieldSize;
    std::string fieldTypeName;
    ASTNode *fieldTypeNode;

    RecordAccessNode(ASTNode *rec, const std::string &fld);
    std::string nodeName() const override { return "RecordAccess"; }
    std::string label() const override;
    std::string annotation() const override;
};

class NumberNode : public ASTNode {
public:
    int value;

    NumberNode(int val);
    std::string nodeName() const override { return "Number"; }
    std::string label() const override;
    std::string annotation() const override;
};

class RealNode : public ASTNode {
public:
    double value;

    RealNode(double val);
    std::string nodeName() const override { return "Real"; }
    std::string label() const override;
    std::string annotation() const override;
};

class BooleanNode : public ASTNode {
public:
    bool value;

    BooleanNode(bool val);
    std::string nodeName() const override { return "Boolean"; }
    std::string label() const override;
    std::string annotation() const override;
};

class StringNode : public ASTNode {
public:
    std::string value;

    StringNode(const std::string &val);
    std::string nodeName() const override { return "String"; }
    std::string label() const override;
    std::string annotation() const override;
};

class CharNode : public ASTNode {
public:
    char value;

    CharNode(char val);
    std::string nodeName() const override { return "Char"; }
    std::string label() const override;
    std::string annotation() const override;
};

class FuncCallNode : public ASTNode {
public:
    std::string funcName;
    std::vector<ASTNode *> args;

    FuncCallNode(const std::string &name, const std::vector<ASTNode *> &arguments);
    std::string nodeName() const override { return "FuncCall"; }
    std::string label() const override;
    std::string annotation() const override;
};

class SubprogramDeclNode : public ASTNode {
public:
    std::string kind;
    std::string subName;
    std::string returnType;
    std::vector<VarDeclNode *> params;
    ASTNode *body;

    SubprogramDeclNode(const std::string &k, const std::string &n,
                       const std::vector<VarDeclNode *> &p,
                       const std::string &retType, ASTNode *bdy);
    std::string nodeName() const override { return "SubprogramDecl"; }
    std::string label() const override;
    std::string annotation() const override;
};

class ArrayTypeNode : public ASTNode {
public:
    ASTNode *indexType;
    ASTNode *elementType;
    int lowBound;
    int highBound;
    int elementSize;
    int totalSize;
    std::string elementTypeName;

    ArrayTypeNode(ASTNode *idxType, ASTNode *elemType);
    std::string nodeName() const override { return "ArrayType"; }
    std::string annotation() const override;
};

class RecordTypeNode : public ASTNode {
public:
    std::vector<VarDeclNode *> fields;
    int totalSize;

    RecordTypeNode(const std::vector<VarDeclNode *> &flds);
    std::string nodeName() const override { return "RecordType"; }
    std::string annotation() const override;
};

class RangeNode : public ASTNode {
public:
    ASTNode *low;
    ASTNode *high;
    int lowValue;
    int highValue;
    bool hasBounds;

    RangeNode(ASTNode *l, ASTNode *h);
    std::string nodeName() const override { return "Range"; }
    std::string annotation() const override;
};

class EnumeratedNode : public ASTNode {
public:
    std::vector<std::string> values;

    EnumeratedNode(const std::vector<std::string> &vals);
    std::string nodeName() const override { return "Enumerated"; }
    std::string label() const override;
    std::string annotation() const override;
};

namespace ASTHelper {
    inline std::string formatAnnotation(const std::string &type, int level, int tabRef) {
        std::ostringstream oss;
        bool hasAny = false;
        oss << "  ->  ";
        if (!type.empty() && type != "void") {
            oss << "type:" << type;
            hasAny = true;
        }
        if (level > 0) {
            if (hasAny)
                oss << ", ";
            oss << "lev:" << level;
            hasAny = true;
        }
        if (tabRef > 0) {
            if (hasAny)
                oss << ", ";
            oss << "tab_index:" << tabRef;
            hasAny = true;
        }
        if (!hasAny)
            return "";
        return oss.str();
    }
}

#endif
