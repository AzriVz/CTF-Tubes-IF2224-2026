#ifndef __PARSER_HPP__
#define __PARSER_HPP__

#include "../Lexer/Lexer.hpp"
#include <string>
#include <vector>
#include <iostream>

struct ParseNode {
    std::string name;
    std::string token;
    std::vector<ParseNode*> children;

    ParseNode(const std::string& n) : name(n) {}

    ~ParseNode() {
        for(auto c : children) delete c;
    }

    void add(ParseNode* c) {
        children.push_back(c);
    }

    void print(std::ostream& out, const std::string& prefix = "", bool isLast = true) {
        out << prefix;
        if (prefix.empty()) {
            out << name;
        } else {
            out << (isLast ? "└── " : "├── ") << name;
        }
        if(!token.empty()) out << "(" << token << ")";
        out << std::endl;
        for (std::size_t i = 0; i < children.size(); i++) {
            children[i]->print(out, prefix + (isLast ? "    " : "│   "), i + 1 == children.size());
        }
    }
};

class Parser {
    private:
        Lexer& lexer;
        Token curr;
        Token advance();
        void expect(TokenType t, const std::string& msg="");

        ParseNode* programHeader();
        ParseNode* declarationPart();
        ParseNode* constDeclaration();
        ParseNode* constant();
        ParseNode* typeDeclaration();
        ParseNode* typeSpecification();
        ParseNode* arrayType();
        ParseNode* recordType();
        ParseNode* fieldList();
        ParseNode* fieldPart();
        ParseNode* subprogramDeclaration();
        ParseNode* procedureDeclaration();
        ParseNode* functionDeclaration();
        ParseNode* compoundStatement();
        ParseNode* statementList();
        ParseNode* statement();
        ParseNode* assignmentStatement();
        ParseNode* variable();
        ParseNode* componentVariable();
        ParseNode* indexList();
        ParseNode* procedureOrFunctionCall();
        ParseNode* parameterList();
        ParseNode* expression();
        ParseNode* simpleExpression();
        ParseNode* term();
        ParseNode* factor();
        ParseNode* rangeType();
        ParseNode* enumeratedType();
        ParseNode* block();
        ParseNode* formalParameterList();
        ParseNode* parameterGroup();
        ParseNode* ifStatement();
        ParseNode* caseStatement();
        ParseNode* caseBlock();
        ParseNode* whileStatement();
        ParseNode* repeatStatement();
        ParseNode* forStatement();
        ParseNode* relationalOperator();
        ParseNode* additiveOperator();
        ParseNode* multiplicativeOperator();

    public:
        Parser(Lexer& l);
        ParseNode* parseProgram();
};

#endif
