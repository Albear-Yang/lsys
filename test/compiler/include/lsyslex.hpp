// lsyslex.hpp
#ifndef LSYSLEX_HPP
#define LSYSLEX_HPP

#include <string>
#include <vector>
#include <stdexcept>
#include <cctype>
#include <unordered_map>

struct Token {
    std::string kind;
    std::string lexeme;
};

inline std::vector<Token> tokenize(const std::string &src) {
    static const std::unordered_map<std::string, std::string> keywords = {
        {"float",      "FLOAT"},
        {"symbol",     "SYMBOL"},
        {"rule",       "RULE"},
        {"draw_rule",  "DRAW_RULE"},
        {"return",     "RETURN"},
        {"if",         "IF"},
        {"else",       "ELSE"},
        {"while",      "WHILE"},
        {"null",       "NULL"},
        {"line",       "LINE"},
        {"turn_pitch", "TURN_PITCH"},
        {"turn_yaw",   "TURN_YAW"},
        {"turn_roll",  "TURN_ROLL"},
        {"fill",       "FILL"},
        {"global",     "GLOBAL"},
        {"when",       "WHEN"},
    };

    static const std::vector<std::pair<std::string,std::string>> symbols = {
        {"->", "ARROW"},
        {"==", "EQ"},
        {"!=", "NE"},
        {"<=", "LE"},
        {">=", "GE"},
        {"<",  "LT"},
        {">",  "GT"},
        {"=",  "BECOMES"},
        {"+",  "PLUS"},
        {"-",  "MINUS"},
        {"*",  "STAR"},
        {"/",  "SLASH"},
        {"%",  "PCT"},
        {"(",  "LPAREN"},
        {")",  "RPAREN"},
        {"{",  "LBRACE"},
        {"}",  "RBRACE"},
        {"[",  "LBRACK"},
        {"]",  "RBRACK"},
        {";",  "SEMI"},
        {",",  "COMMA"},
        {":",  "COLON"},
    };

    std::vector<Token> tokens;
    size_t i = 0;
    // true when the last emitted token could be the end of a value-producing
    // expression — used to distinguish unary minus from binary minus
    bool last_was_value = false;

    while (i < src.size()) {
        // skip whitespace
        if (std::isspace(src[i])) { ++i; continue; }

        // single-line comment
        if (src[i] == '/' && i+1 < src.size() && src[i+1] == '/') {
            while (i < src.size() && src[i] != '\n') ++i;
            continue;
        }

        // unary minus: '-' not preceded by a value-producing token, followed by a digit
        if (src[i] == '-' && !last_was_value &&
            i+1 < src.size() && (std::isdigit(src[i+1]) ||
            (src[i+1] == '.' && i+2 < src.size() && std::isdigit(src[i+2])))) {
            size_t start = i;
            ++i; // consume '-'
            while (i < src.size() && std::isdigit(src[i])) ++i;
            if (i < src.size() && src[i] == '.') {
                ++i;
                while (i < src.size() && std::isdigit(src[i])) ++i;
            }
            tokens.push_back({"NUM", src.substr(start, i - start)});
            last_was_value = true;
            continue;
        }

        // number literal: digits, optional decimal point
        if (std::isdigit(src[i]) || (src[i] == '.' && i+1 < src.size() && std::isdigit(src[i+1]))) {
            size_t start = i;
            while (i < src.size() && std::isdigit(src[i])) ++i;
            if (i < src.size() && src[i] == '.') {
                ++i;
                while (i < src.size() && std::isdigit(src[i])) ++i;
            }
            tokens.push_back({"NUM", src.substr(start, i - start)});
            last_was_value = true;
            continue;
        }

        // identifier or keyword
        if (std::isalpha(src[i]) || src[i] == '_') {
            size_t start = i;
            while (i < src.size() && (std::isalnum(src[i]) || src[i] == '_')) ++i;
            std::string word = src.substr(start, i - start);
            auto it = keywords.find(word);
            tokens.push_back(it != keywords.end()
                ? Token{it->second, word}
                : Token{"ID", word});
            // ID and RPAREN can end a value; keywords cannot
            last_was_value = (it == keywords.end()); // true only for ID
            continue;
        }

        // multi-char and single-char symbols (check longer ones first)
        bool matched = false;
        for (auto &[sym, kind] : symbols) {
            if (src.substr(i, sym.size()) == sym) {
                tokens.push_back({kind, sym});
                i += sym.size();
                // only RPAREN ends a value-producing expression
                last_was_value = (kind == "RPAREN");
                matched = true;
                break;
            }
        }
        if (matched) continue;

        throw std::runtime_error(
            "Unrecognised character '" + std::string(1, src[i]) + "' at position " + std::to_string(i));
    }

    return tokens;
}

// Converts a token list to the line-per-token format the parser expects:
// "KIND lexeme\n" per token, wrapped in BOF/EOF
inline std::string tokens_to_parser_input(const std::vector<Token> &tokens) {
    std::string out;
    for (auto &t : tokens)
        out += t.kind + " " + t.lexeme + "\n";
    return out;
}

#endif // LSYSLEX_HPP