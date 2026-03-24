#include <iostream>
#include <fstream>
#include <sstream>
#include "lexer/lexer.h"

// Helper: Convert TokenType to string (for debug)
std::string tokenTypeToString(TokenType type) {
    switch (type) {
        case TokenType::BIND: return "BIND";
        case TokenType::IS: return "IS";
        case TokenType::EMIT: return "EMIT";
        case TokenType::CHECK: return "CHECK";
        case TokenType::OTHERWISE: return "OTHERWISE";
        case TokenType::CYCLE: return "CYCLE";
        case TokenType::FORGE: return "FORGE";
        case TokenType::YIELD: return "YIELD";

        case TokenType::TYPE_INT: return "TYPE_INT";
        case TokenType::TYPE_BOOL: return "TYPE_BOOL";

        case TokenType::INT_LITERAL: return "INT_LITERAL";
        case TokenType::BOOL_LITERAL: return "BOOL_LITERAL";

        case TokenType::IDENTIFIER: return "IDENTIFIER";

        case TokenType::PLUS: return "PLUS";
        case TokenType::MINUS: return "MINUS";
        case TokenType::MULTIPLY: return "MULTIPLY";
        case TokenType::DIVIDE: return "DIVIDE";

        case TokenType::GREATER: return "GREATER";
        case TokenType::LESS: return "LESS";
        case TokenType::GREATER_EQUAL: return "GREATER_EQUAL";
        case TokenType::LESS_EQUAL: return "LESS_EQUAL";
        case TokenType::EQUAL: return "EQUAL";
        case TokenType::NOT_EQUAL: return "NOT_EQUAL";

        case TokenType::ASSIGN: return "ASSIGN";

        case TokenType::COLON: return "COLON";
        case TokenType::SEMICOLON: return "SEMICOLON";
        case TokenType::COMMA: return "COMMA";

        case TokenType::LPAREN: return "LPAREN";
        case TokenType::RPAREN: return "RPAREN";
        case TokenType::LBRACE: return "LBRACE";
        case TokenType::RBRACE: return "RBRACE";

        case TokenType::END_OF_FILE: return "EOF";
    }

    return "UNKNOWN";
}

// Read file content
std::string readFile(const std::string& path) {
    std::ifstream file(path);
    if (!file) {
        throw std::runtime_error("Cannot open file: " + path);
    }

    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

int main(int argc, char* argv[]) {
    std::cout << "LiveLy Compiler Starting...\n";

    if (argc < 2) {
        std::cout << "Usage: lively <source_file.lv>\n";
        return 1;
    }

    try {
        std::string source = readFile(argv[1]);

        Lexer lexer(source);
        std::vector<Token> tokens = lexer.tokenize();

        std::cout << "\n--- TOKENS ---\n";

        for (const auto& token : tokens) {
            std::cout << tokenTypeToString(token.type)
                      << " (" << token.value << ") "
                      << "[Line: " << token.line
                      << ", Col: " << token.column << "]\n";
        }

    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
    }

    return 0;
}