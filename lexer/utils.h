#ifndef LEXER_UTILS_H
#define LEXER_UTILS_H

#include <queue>
#include <set>
#include <string>
#include <vector>

namespace parser {
namespace AST {
class Node;
class PatternNode;
}  // namespace AST
}  // namespace parser

namespace lexer {
struct EpsTrans {
	std::size_t to;
	EpsTrans* next;
};

struct Tables {
	std::size_t* next;
	// contains accept actions (1-index of given vector, because then can test presence of acceptance just with boolean)
	std::size_t* accept;
	EpsTrans** epsilon;

	std::size_t numStates, maxStates;
};

Tables generateTables(const std::vector<parser::AST::PatternNode*>& rules);

std::size_t makeState(Tables& tables);

void makeTransition(Tables& tables, std::size_t from, std::size_t to, unsigned char c);

void makeAccept(Tables& tables, std::size_t state, std::size_t idx);

void makeNFA(Tables& tables, parser::AST::Node* regex, std::size_t start, std::size_t end);

std::set<std::size_t> findEpsClosure(const Tables& nfa, std::size_t state);

void expand(Tables& tables);

void expand(std::set<std::size_t>*& map, std::size_t& size, std::size_t target);

void freeTables(const Tables& tables);
}  // namespace lexer

#endif