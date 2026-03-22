#include "Lexer.hpp"
#include <iostream>
#include <sstream>
#include <cctype>
#include <fstream>
#include <stdexcept>
using namespace std;

Lexer::Lexer(const string& filename) : currPos(0), currLine(1), currCol(1) {
    ifstream file(filename);
    if (!file.is_open()) {
        throw runtime_error(filename + " tidak bisa dibuka.");
        currChar = '\0';
        return;
    }
    stringstream buffer;
    buffer << file.rdbuf();
    source = buffer.str();

    if (source.length() > 0) {
        currChar = source[0];
    } else {
        currChar = '\0';
    }

    keywords["const"] = TokenType::CONSTSY;
    keywords["type"] = TokenType::TYPESY;
    keywords["var"] = TokenType::VARSY;
    keywords["function"] = TokenType::FUNCTIONSY;
    keywords["procedure"] = TokenType::PROCEDURESY;
    keywords["array"] = TokenType::ARRAYSY;
    keywords["record"] = TokenType::RECORDSY;
    keywords["program"] = TokenType::PROGRAMSY;
    keywords["begin"] = TokenType::BEGINSY;
    keywords["if"] = TokenType::IFSY;
    keywords["case"] = TokenType::CASESY;
    keywords["repeat"] = TokenType::REPEATSY;
    keywords["while"] = TokenType::WHILESY;
    keywords["for"] = TokenType::FORSY;
    keywords["end"] = TokenType::ENDSY;
    keywords["else"] = TokenType::ELSESY;
    keywords["until"] = TokenType::UNTILSY;
    keywords["of"] = TokenType::OFSY;
    keywords["do"] = TokenType::DOSY;
    keywords["to"] = TokenType::TOSY;
    keywords["downto"] = TokenType::DOWNTOSY;
    keywords["then"] = TokenType::THENSY;
    keywords["not"] = TokenType::NOTSY;
    keywords["and"] = TokenType::ANDSY;
    keywords["or"] = TokenType::ORSY;
    keywords["mod"] = TokenType::IMOD;
    keywords["div"] = TokenType::IDIV;
}

void Lexer::advance() {
    currPos++;
    if (currPos > source.length()-1) {
        currChar = '\0';
    }
    else {
        currChar = source[currPos];
        currCol++;
        if (currChar == '\n') {
            currLine++;
            currCol = 0;
        }
    }
}

char Lexer::peek() {
    if (currPos + 1 > source.length()-1) {
        return '\0';
    }
    else {
        return source[currPos + 1];
    }
}

void Lexer::skipWhiteSpace() {
    while (currChar != '\0' && isspace(currChar)) {
        advance();
    }
}

string Lexer::toLowerCase(const string& str) {
    string lowerStr = str;
    for (char& c : lowerStr) {
        c = tolower(c);
    }
    return lowerStr;
}

Token Lexer::identOrKeyword() {
    string result = "";
    int startCol = currCol;
    while (currChar != '\0' && isalnum(currChar)) {
        result += currChar;
        advance();
    }
    string lowerResult = toLowerCase(result);
    TokenType type = TokenType::IDENT;

    if (keywords.find(lowerResult) != keywords.end()) {
        type = keywords[lowerResult];
    }
    return {type, result, currLine, startCol};
}

Token Lexer::number() {
    string result = "";
    int startCol = currCol;
    while (currChar != '\0' && isdigit(currChar)) {
        result += currChar;
        advance();
    }

    if (currChar == '.') {
        if (isdigit(peek())) {
            result += currChar;
            advance();
            while(currChar != '\0' && isdigit(currChar)) {
                result += currChar;
                advance();
            }
            return {TokenType::REALCON,result, currLine, startCol};
        }
    }
    return {TokenType::INTCON, result, currLine, startCol};
}

Token Lexer::stringOrChar() {
    string result = "";
    int startCol = currCol;

    advance();
    while (currChar != '\0') {
        if(currChar == '\'') {
            if(peek() == '\'') {
                result += '\'';
                advance();
                advance();
            }
            else {
                advance();
                break;
            }
        }
        else {
            result += currChar;
            advance();
        }
    }

    TokenType type;
    if(result.length() == 1) {
        type = TokenType::CHARCON;
    }
    else {
        type = TokenType::STRING;
    }

    return {type, result, currLine, startCol};
}

Token Lexer::commentCurly() {
    string result = "{";
    int startCol = currCol;
    
    advance();
    while(currChar != '\0' && currChar != '}') {
        result += currChar;
        advance();
    }

    if(currChar == '}') {
        result += '}';
        advance();
    }
    return {TokenType::COMMENT, result, currLine, startCol};
}

Token Lexer::commentParentheses() {
    string result = "(*";
    int startCol = currCol;
    advance();
    advance();
    while(currChar != '\0') {
        if(currChar == '*' && peek() == ')') {
            result += "*)";
            advance();
            advance();
            break;
        }
        result += currChar;
        advance();
    }
    return{TokenType::COMMENT, result, currLine, startCol};
}

Token Lexer::getNextToken() {
    while(currChar != '\0') {
        if(isspace(currChar)) {
            skipWhiteSpace();
            continue;
        }
        int startCol = currCol;
        if(isalpha(currChar)) {
            return identOrKeyword();
        }
        if(isdigit(currChar)) {
            return number();
        }
        if(currChar == '<') {
            advance();
            if(currChar == '=') {
                advance();
                return {TokenType::LEQ, "<=", currLine, startCol};
            }
            if(currChar == '>') {
                advance();
                return {TokenType::NEQ, "<>", currLine, startCol};
            }
            return {TokenType::LSS, "<", currLine, startCol}; 
        }
        if(currChar == '>') {
            advance();
            if(currChar == '=') {
                advance();
                return {TokenType::GEQ, ">=", currLine, startCol};
            }
            return {TokenType::GTR, ">", currLine, startCol};
        }
        if(currChar == ':') {
            advance();
            if(currChar == '=') {
                advance();
                return {TokenType::BECOMES, ":=", currLine, startCol};
            }
            return {TokenType::COLON, ":", currLine, startCol};
        }
        if(currChar == '=') {
            advance();
            if(currChar == '=') {
                advance();
                return {TokenType::EQL, "==", currLine, startCol};
            }
            return {TokenType::UNKNOWN, "=", currLine, startCol};
        }
        if(currChar == '\'') {
            return stringOrChar();
        }
        if(currChar == '{') {
            return commentCurly();
        }
        if(currChar == '(') {
            if(peek() == '*') {
                return commentParentheses();
            }
            advance();
            return {TokenType::LPARENT, "(", currLine, startCol};

        }

        char c = currChar;
        advance();
        if(c == '+') {
            return {TokenType::PLUS, "+", currLine, startCol};
        }
        else if(c == '-') {
            return {TokenType::MINUS, "-", currLine, startCol};
        }
        else if(c == '*') {
            return {TokenType::TIMES, "*", currLine, startCol};
        }
        else if(c == '/') {
            return {TokenType::RDIV, "/", currLine, startCol};
        }
        else if(c == ')') {
            return {TokenType::RPARENT, ")", currLine, startCol};
        }
        else if(c == '[') {
            return {TokenType::LBRACK, "[", currLine, startCol};
        }
        else if(c == ']') {
            return {TokenType::RBRACK, "]", currLine, startCol};
        }
        else if(c == ',') {
            return {TokenType::COMMA, ",", currLine, startCol};
        }
        else if(c == '.') {
            return {TokenType::PERIOD, ".", currLine, startCol};
        }
        else if(c == ';') {
            return {TokenType::SEMICOLON, ";", currLine, startCol};
        }
        else {
            string unknown(1, c);
            return {TokenType::UNKNOWN, unknown, currLine, startCol};
        }
    }
    return {TokenType::END_OF_FILE, "EOF", currLine, currCol};
}

string Lexer::tokenTypeToString(TokenType type) {
    if(type == TokenType::INTCON) {
        return "intcon";
    }
    else if(type == TokenType::REALCON) {
        return "realcon";
    }
    else if(type == TokenType::CHARCON) {
        return "charcon";
    }
    else if(type == TokenType::STRING) {
        return "string";
    }
    else if(type == TokenType::NOTSY) {
        return "notsy";
    }
    else if(type == TokenType::PLUS) {
        return "plus";
    }
    else if(type == TokenType::MINUS) {
        return "minus";
    }
    else if(type == TokenType::TIMES) {
        return "times";
    }
    else if(type == TokenType::IDIV) {
        return "idiv";
    }
    else if(type == TokenType::RDIV) {
        return "rdiv";
    }
    else if(type == TokenType::IMOD) {
        return "imod";
    }
    else if(type == TokenType::ANDSY) {
        return "andsy";
    }
    else if(type == TokenType::ORSY) {
        return "orsy";
    }
    else if(type == TokenType::EQL) {
        return "eql";
    }
    else if(type == TokenType::NEQ) {
        return "neq";
    }
    else if(type == TokenType::GTR) {
        return "gtr";
    }
    else if(type == TokenType::GEQ) {
        return "geq";
    }
    else if(type == TokenType::LSS) {
        return "lss";
    }
    else if(type == TokenType::LEQ) {
        return "leq";
    }
    else if(type == TokenType::LPARENT) {
        return "lparent";
    }
    else if(type == TokenType::RPARENT) {
        return "rparent";
    }
    else if(type == TokenType::LBRACK) {
        return "lbrack";
    }
    else if(type == TokenType::RBRACK) {
        return "rbrack";
    }
    else if(type == TokenType::COMMA) {
        return "comma";
    }
    else if(type == TokenType::SEMICOLON) {
        return "semicolon";
    }
    else if(type == TokenType::PERIOD) {
        return "period";
    }
    else if(type == TokenType::COLON) {
        return "colon";
    }
    else if(type == TokenType::BECOMES) {
        return "becomes";
    }
    else if(type == TokenType::CONSTSY) {
        return "constsy";
    }
    else if(type == TokenType::TYPESY) {
        return "typesy";
    }
    else if(type == TokenType::VARSY) {
        return "varsy";
    }
    else if(type == TokenType::FUNCTIONSY) {
        return "functionsy";
    }
    else if(type == TokenType::PROCEDURESY) {
        return "proceduresy";
    }
    else if(type == TokenType::ARRAYSY) {
        return "arraysy";
    }
    else if(type == TokenType::RECORDSY) {
        return "recordsy";
    }
    else if(type == TokenType::PROGRAMSY) {
        return "programsy";
    }
    else if(type == TokenType::IDENT) {
        return "ident";
    }
    else if(type == TokenType::BEGINSY) {
        return "beginsy";
    }
    else if(type == TokenType::IFSY) {
        return "ifsy";
    }
    else if(type == TokenType::CASESY) {
        return "casesy";
    }
    else if(type == TokenType::REPEATSY) {
        return "repeatsy";
    }
    else if(type == TokenType::WHILESY) {
        return "whilesy";
    }
    else if(type == TokenType::FORSY) {
        return "forsy";
    }
    else if(type == TokenType::ENDSY) {
        return "endsy";
    }
    else if(type == TokenType::ELSESY) {
        return "elsesy";
    }
    else if(type == TokenType::UNTILSY) {
        return "untilsy";
    }
    else if(type == TokenType::OFSY) {
        return "ofsy";
    }
    else if(type == TokenType::DOSY) {
        return "dosy";
    }
    else if(type == TokenType::TOSY) {
        return "tosy";
    }
    else if(type == TokenType::DOWNTOSY) {
        return "downtosy";
    }
    else if(type == TokenType::THENSY) {
        return "thensy";
    }
    else if(type == TokenType::COMMENT) {
        return "comment";
    }
    else if(type == TokenType::END_OF_FILE) {
        return "end_of_file";
    }
    else if(type == TokenType::UNKNOWN) {
        return "unknown";
    }
    else {
        return "error";
    }
}