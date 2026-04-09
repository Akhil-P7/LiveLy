#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>

#include "lexer/lexer.h"
#include "parser/parser.h"
#include "ast/ast_printer.h"
#include "semantic/semantic.h"

// Helper: Convert TokenType to a human-readable string.
std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::BIND:           return "BIND";
        case TokenType::IS:             return "IS";
        case TokenType::EMIT:           return "EMIT";
        case TokenType::CHECK:          return "CHECK";
        case TokenType::OTHERWISE:      return "OTHERWISE";
        case TokenType::CYCLE:          return "CYCLE";
        case TokenType::FORGE:          return "FORGE";
        case TokenType::YIELD:          return "YIELD";

        case TokenType::TYPE_INT:       return "TYPE_INT";
        case TokenType::TYPE_BOOL:      return "TYPE_BOOL";

        case TokenType::INT_LITERAL:    return "INT_LITERAL";
        case TokenType::BOOL_LITERAL:   return "BOOL_LITERAL";

        case TokenType::IDENTIFIER:     return "IDENTIFIER";

        case TokenType::PLUS:           return "PLUS";
        case TokenType::MINUS:          return "MINUS";
        case TokenType::MULTIPLY:       return "MULTIPLY";
        case TokenType::DIVIDE:         return "DIVIDE";

        case TokenType::GREATER:        return "GREATER";
        case TokenType::LESS:           return "LESS";
        case TokenType::GREATER_EQUAL:  return "GREATER_EQUAL";
        case TokenType::LESS_EQUAL:     return "LESS_EQUAL";
        case TokenType::EQUAL:          return "EQUAL";
        case TokenType::NOT_EQUAL:      return "NOT_EQUAL";

        case TokenType::ASSIGN:         return "ASSIGN";

        case TokenType::COLON:          return "COLON";
        case TokenType::SEMICOLON:      return "SEMICOLON";
        case TokenType::COMMA:          return "COMMA";

        case TokenType::LPAREN:         return "LPAREN";
        case TokenType::RPAREN:         return "RPAREN";
        case TokenType::LBRACE:         return "LBRACE";
        case TokenType::RBRACE:         return "RBRACE";

        case TokenType::END_OF_FILE:    return "EOF";
    }

    return "UNKNOWN";
}

// Read entire file contents into a string.
std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Cannot open file: " + path);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

// Main — LiveLy Compiler Pipeline
// Source → Lexer (Tokens) → Parser (AST) → AST Print → Semantic Analysis
int main(int argc, char* argv[]) {
    std::cout << "====================================\n";
    std::cout << "   LiveLy Compiler v0.3             \n";
    std::cout << "====================================\n";

    if (argc < 2) {
        std::cout << "Usage: lively <source_file.lv>\n";
        return 1;
    }

    try {
        // PHASE 1: Read Source
        std::string source = readFile(argv[1]);
        std::cout << "\n[Phase 1] Source loaded: " << argv[1] << "\n";

        // PHASE 2: Lexical Analysis
        Lexer lexer(source);
        std::vector<Token> tokens = lexer.tokenize();

        std::cout << "[Phase 2] Lexer produced " << tokens.size() << " tokens.\n";
        std::cout << "\n--- TOKEN STREAM ---\n";

        for (const auto& token : tokens) {
            std::cout << "  " << tokenTypeToString(token.type)
                      << " (" << token.value << ") "
                      << "[Line: " << token.line
                      << ", Col: " << token.column << "]\n";
        }

        // PHASE 3: Parsing → AST
        Parser parser(tokens);
        auto ast = parser.parse();

        std::cout << "\n[Phase 3] Parser produced " << ast.size()
                  << " top-level statement" << (ast.size() != 1 ? "s" : "")
                  << ".\n";

        // PHASE 4: AST Visualization
        ASTPrinter::printProgram(ast);

        // PHASE 5: Semantic Analysis
        SemanticAnalyzer analyzer;
        analyzer.analyze(ast);

        std::cout << "\n[Phase 5] Semantic analysis passed — all types valid.\n";

        std::cout << "\n[OK] All compiler phases completed successfully.\n";

    } catch (const std::exception& e) {
        std::cerr << "\n[FATAL ERROR] " << e.what() << std::endl;
        return 1;
    }

    return 0;
}