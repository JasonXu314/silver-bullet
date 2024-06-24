#ifndef PARSER_UTILS_H
#define PARSER_UTILS_H

#include <iomanip>
#include <iostream>

#include "../lexer.h"
#include "ast.h"

namespace parser {
AST::PatternNode* parsePattern(lexer::TokenStream& tokens);

AST::TokenNode* parseToken(lexer::TokenStream& tokens);

AST::RegexNode* parseRegex(lexer::TokenStream& tokens);

AST::RegexRangeNode* parseRange(lexer::TokenStream& tokens);
}  // namespace parser

std::ostream& indent(std::ostream& stream, unsigned int level);

#endif