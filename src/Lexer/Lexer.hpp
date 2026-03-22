#ifndef __LEXER_HPP__
#define __LEXER_HPP__

#include <vector>
#include <string>
#include <unordered_map>

enum class TokenType {
    INTCON,
    REALCON,
    CHARCON,
    STRING,
    NOTSY,
    PLUS,
    MINUS,
    TIMES,
    IDIV,
    RDIV,
    IMOD,
    ANDSY,
    ORSY,
    EQL,
    NEQ,
    GTR,
    GEQ,
    LSS,
    LEQ,
    LPARENT,
    RPARENT,
    LBRACK,
    RBRACK,
    COMMA,
    SEMICOLON,
    PERIOD,
    COLON,
    BECOMES,
    CONSTSY,
    TYPESY,
    VARSY,
    FUNCTIONSY,
    PROCEDURESY,
    ARRAYSY,
    RECORDSY,
    PROGRAMSY,
    IDENT,
    BEGINSY,
    IFSY,
    CASESY,
    REPEATSY,
    WHILESY,
    FORSY,
    ENDSY,
    ELSESY,
    UNTILSY,
    OFSY,
    DOSY,
    TOSY,
    DOWNTOSY,
    THENSY,
    COMMENT,
    END_OF_FILE,
    UNKNOWN
};

struct Token {
    TokenType type;
    std::string value;
    int line;
    int column;
};

class Lexer {
    private:
        std::string source;
        int currPos;
        char currChar;
        int currLine;
        int currCol;
        std::unordered_map<std::string, TokenType> keywords;

        void advance();
        char peek();
        void skipWhiteSpace();
        std::string toLowerCase(const std::string& str);

        Token number();
        Token identOrKeyword();
        Token stringOrChar();
        Token commentCurly();
        Token commentParentheses();
    public:
        Lexer(const std::string& filename);
        Token getNextToken();
        std::string tokenTypeToString(TokenType type);
};

#endif