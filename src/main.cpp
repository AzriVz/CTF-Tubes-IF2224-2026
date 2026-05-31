#include "Lexer/Lexer.hpp"
#include "Parser/Parser.hpp"
#include "Semantic/SemanticAnalyzer.hpp"
#include "Intermediate/CodeGenerator.hpp"
#include <string>
#include <iostream>
#include <fstream>
using namespace std;

int main(int argc, char *argv[]) {
    if (argc < 2) {
        cerr << "Usage: ./bin/arion-compiler [--parse|--semantic|--ic] <input-file.txt>" << endl;
        cerr << "  Default: lexer only" << endl;
        cerr << "  --parse: run lexer + parser and output parse tree" << endl;
        cerr << "  --semantic: run parser + semantic analysis and output decorated AST + symbol tables" << endl;
        cerr << "  --ic: run parser + semantic analysis + intermediate code generation" << endl;
        cerr << "  Example: ./bin/arion-compiler test.txt" << endl;
        cerr << "  Example: ./bin/arion-compiler --parse test.txt" << endl;
        cerr << "  Example: ./bin/arion-compiler --semantic test.txt" << endl;
        cerr << "  Example: ./bin/arion-compiler --ic test.txt" << endl;
        return 1;
    }

    bool parseMode = false;
    bool semanticMode = false;
    bool intermediateMode = false;
    string filename;

    if (argc >= 3) {
        if (string(argv[1]) == "--parse") {
            parseMode = true;
            filename = argv[2];
        }
        else if (string(argv[1]) == "--semantic") {
            semanticMode = true;
            filename = argv[2];
        }
        else if (string(argv[1]) == "--ic") {
            intermediateMode = true;
            filename = argv[2];
        }
        else {
            filename = argv[1];
        }
    }
    else {
        filename = argv[1];
    }

    try {
        if (parseMode) {
            // Parser mode
            Lexer lexer(filename);
            Parser parser(lexer);
            ParseNode *tree = parser.parseProgram();

            // Print tree to stdout
            tree->print(cout);

            // Save tree to output file
            string outfile = filename;
            size_t lastDot = outfile.rfind('.');
            if (lastDot != string::npos) {
                outfile = outfile.substr(0, lastDot) + "_parse.txt";
            }
            else {
                outfile = outfile + "_parse.txt";
            }
            ofstream out(outfile);
            tree->print(out);
            out.close();
            cerr << "Parse tree saved to: " << outfile << endl;

            delete tree;
        }
        else if (semanticMode) {
            // Semantic Analysis mode
            Lexer lexer(filename);
            Parser parser(lexer);
            ParseNode *tree = parser.parseProgram();

            // Run semantic analysis
            SemanticAnalyzer analyzer;
            ASTNode *ast = analyzer.analyze(tree);

            // Print results to stdout
            analyzer.printResults(cout, ast);

            // Save results to output file
            string outfile = filename;
            size_t lastDot = outfile.rfind('.');
            if (lastDot != string::npos) {
                outfile = outfile.substr(0, lastDot) + "_semantic.txt";
            }
            else {
                outfile = outfile + "_semantic.txt";
            }
            ofstream out(outfile);
            analyzer.printResults(out, ast);
            out.close();
            cerr << "Semantic analysis saved to: " << outfile << endl;

            delete ast;
            delete tree;
        }
        else if (intermediateMode) {
            // Intermediate Code mode
            Lexer lexer(filename);
            Parser parser(lexer);
            ParseNode *tree = parser.parseProgram();

            SemanticAnalyzer analyzer;
            ASTNode *ast = analyzer.analyze(tree);

            if (analyzer.hasErrors()) {
                cerr << "Semantic analysis failed; intermediate code was not generated." << endl;
                for (const auto &err : analyzer.getErrors()) {
                    cerr << err << endl;
                }
                delete ast;
                delete tree;
                return 1;
            }

            CodeGenerator generator;
            generator.generate(ast, analyzer.getSymbolTable());

            generator.print(cout);

            string outfile = filename;
            size_t lastDot = outfile.rfind('.');
            if (lastDot != string::npos) {
                outfile = outfile.substr(0, lastDot) + "_ic.txt";
            }
            else {
                outfile = outfile + "_ic.txt";
            }
            ofstream out(outfile);
            generator.print(out);
            out.close();
            cerr << "Intermediate code saved to: " << outfile << endl;

            bool hasGenerationErrors = generator.hasErrors();
            delete ast;
            delete tree;

            if (hasGenerationErrors) {
                return 1;
            }
        }
        else {
            // Lexer mode (default)
            Lexer lexer(filename);
            Token token = lexer.getNextToken();
            while (token.type != TokenType::END_OF_FILE) {
                string typeName = lexer.tokenTypeToString(token.type);

                if (token.type == TokenType::IDENT || token.type == TokenType::INTCON || token.type == TokenType::REALCON) {
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
    catch (const exception &e) {
        cerr << "Error: " << e.what() << endl;
        return 1;
    }
    return 0;
}
