#ifndef PARSER_UTILS_H
#define PARSER_UTILS_H

#include <iomanip>
#include <iostream>
#include <map>
#include <string>

#include "../lexer.h"
#include "ast.h"

namespace parser {
AST::PatternNode* parsePatternDecl(lexer::TokenStream& tokens);

AST::TokenNode* parseTokenDecl(lexer::TokenStream& tokens);

AST::RegexNode* parseRegex(lexer::TokenStream& tokens);

AST::RegexRangeNode* parseRange(lexer::TokenStream& tokens);

AST::RuleNode* parseRuleDecl(lexer::TokenStream& tokens);

AST::BodyNode* parseRuleBody(lexer::TokenStream& tokens, char end = '\0');

AST::InternalNode* parse(lexer::TokenStream& tokens, AST::RuleNode* rule, const std::map<std::string, AST::RuleNode*>& rules);

AST::InternalNode* parse(lexer::TokenStream& tokens, const std::string& name, AST::BodyNode* body, const std::map<std::string, AST::RuleNode*>& rules);

namespace utils {
// FIXME: does not handle left-recursion
std::set<std::string> findFIRSTSet(AST::Node* node, const std::map<std::string, AST::RuleNode*>& rules);
}  // namespace utils
}  // namespace parser

std::ostream& indent(std::ostream& stream, unsigned int level);

#endif