#include "Lexer/Lexer.hpp"
#include <string>
#include <iostream>
using namespace std;

int main(int argc, char* argv[]) {
    if(argc < 2) {
        cerr << "Format: '/bin/arion-compiler /test/milestone-X/<input-file.txt>' atau bisa menggunakan 'make run'" << endl;
        return 1;
    }    
    string filename = argv[1]; 
    try {
        Lexer lexer(filename);
        Token token = lexer.getNextToken();        
        while(token.type != TokenType::END_OF_FILE) {
            string typeName = lexer.tokenTypeToString(token.type);
            
            if(token.type == TokenType::IDENT || token.type == TokenType::INTCON || token.type == TokenType::REALCON) {
                cout << typeName << " (" << token.value << ")" << endl;
            }
            else if (token.type == TokenType::STRING || token.type == TokenType::CHARCON) {
                cout << typeName << " ('" << token.value << "')" << endl;
            }
            else if (token.type == TokenType::UNKNOWN) {
                cout << "Karakter tidak dikenal '" << token.value << "' pada Baris " << token.line << ", Kolom " << token.column << endl; 
            }
            else if(token.type != TokenType::COMMENT) {
                cout << typeName << endl;
            }            
            token = lexer.getNextToken(); 
        }
    }
    catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    return 0;
}