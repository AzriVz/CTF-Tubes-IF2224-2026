#include "Lexer/Lexer.hpp"
#include "Parser/Parser.hpp"
#include <string>
#include <iostream>
#include <fstream>
using namespace std;

int main(int argc, char* argv[]) {
    if(argc < 2) {
        cerr << "Usage: ./bin/arion-compiler [--parse] <input-file.txt>" << endl;
        cerr << "  Default: lexer only" << endl;
        cerr << "  --parse: run lexer + parser and output parse tree" << endl;
        cerr << "  Example: ./bin/arion-compiler test.txt" << endl;
        cerr << "  Example: ./bin/arion-compiler --parse test.txt" << endl;
        return 1;
    }
    
    bool parseMode = false;
    string filename;
    
    if(argc >= 3 && string(argv[1]) == "--parse") {
        parseMode = true;
        filename = argv[2];
    } else {
        filename = argv[1];
    }
    
    try {
        if(parseMode) {
            // Parser mode
            Lexer lexer(filename);
            Parser parser(lexer);
            ParseNode* tree = parser.parseProgram();
            
            // Print tree to stdout
            tree->print(cout);
            
            // Save tree to output file
            string outfile = filename;
            size_t lastDot = outfile.rfind('.');
            if(lastDot != string::npos) {
                outfile = outfile.substr(0, lastDot) + "_parse.txt";
            } else {
                outfile = outfile + "_parse.txt";
            }
            ofstream out(outfile);
            tree->print(out);
            out.close();
            cerr << "Parse tree saved to: " << outfile << endl;
            
            delete tree;
        } else {
            // Lexer mode (default)
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
                    cout << typeName << " (" << token.value << ")" << endl;
                }
                else {
                    cout << typeName << endl;
                }            
                token = lexer.getNextToken(); 
            }
        }
    }
    catch (const exception& e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    return 0;
}
