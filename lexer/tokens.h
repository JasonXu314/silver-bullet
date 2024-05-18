#ifndef LEXER_TOKENS_H
#define LEXER_TOKENS_H

#include <cctype>
#include <cmath>
#include <iostream>
#include <string>

namespace lexer {
struct Token {
	std::string type;
	std::string raw;
};
}  // namespace lexer

#endif