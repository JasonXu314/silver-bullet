#include "utils.h"

#include <cstring>

#include "../parser/ast.h"

using namespace std;

lexer::Tables lexer::generateTables(const vector<parser::AST::TokenNode*>& rules) {
	Tables dfa, nfa;

	nfa.numStates = 0;
	nfa.maxStates = 64;

	nfa.next = new size_t[64 * 256];
	nfa.accept = new size_t[64];
	nfa.epsilon = new EpsTrans*[64];

	bzero(nfa.next, 64 * 256 * sizeof(size_t));
	bzero(nfa.accept, 64 * sizeof(size_t));
	bzero(nfa.epsilon, 64 * sizeof(EpsTrans*));

	size_t nfaTrap = makeState(nfa), nfaStart = makeState(nfa);

	for (size_t i = 0; i < rules.size(); i++) {
		auto regex = rules[i];
		// cout << "Generating rule " << regex->name() << endl;

		size_t start = makeState(nfa), end = makeState(nfa);
		makeAccept(nfa, end, i + 1);

		makeNFA(nfa, regex->children()[0], start, end);

		makeTransition(nfa, nfaStart, start, EOF);
	}

	dfa.numStates = 0;
	dfa.maxStates = nfa.numStates * nfa.numStates;

	dfa.next = new size_t[dfa.maxStates * 256];
	dfa.accept = new size_t[dfa.maxStates];
	dfa.epsilon = nullptr;

	bzero(dfa.next, dfa.maxStates * 256 * sizeof(size_t));
	bzero(dfa.accept, dfa.maxStates * sizeof(size_t));

	// for (size_t state = 0; state < nfa.numStates; state++) {
	// cout << "state " << state << " epsilon-closure: ";

	// for (size_t s : findEpsClosure(nfa, state)) {
	// 	cout << s << " ";
	// }

	// cout << endl;
	// }

	size_t mapCap = dfa.maxStates;
	set<size_t>* closureMap = new set<size_t>[mapCap];
	size_t dfaTrap = makeState(dfa), dfaStart = makeState(dfa);
	closureMap[dfaTrap].emplace(nfaTrap);

	queue<size_t> frontier;

	for (size_t s : findEpsClosure(nfa, nfaStart)) {
		closureMap[dfaStart].emplace(s);
	}

	frontier.push(dfaStart);

	while (!frontier.empty()) {
		size_t next = frontier.front();
		frontier.pop();
		set<size_t> closure = closureMap[next];

		// use 255 (EOF) for eps transitions
		for (int c = 0; c < 255; c++) {
			set<size_t> resultClosure;

			for (size_t state : closure) {
				for (size_t nextState : findEpsClosure(nfa, nfa.next[state * 256 + c])) {
					resultClosure.emplace(nextState);
				}
			}

			size_t nextState = (size_t)-1;
			for (size_t i = 0; i < dfa.numStates; i++) {
				if (closureMap[i] == resultClosure) {
					nextState = i;
					break;
				}
			}

			if (nextState == (size_t)-1) {
				nextState = makeState(dfa);

				if (mapCap != dfa.maxStates) {
					expand(closureMap, mapCap, dfa.maxStates);
				}

				closureMap[nextState] = resultClosure;

				makeTransition(dfa, next, nextState, c);
				frontier.push(nextState);
			} else {
				makeTransition(dfa, next, nextState, c);
			}

			for (size_t state : resultClosure) {
				if (nfa.accept[state] && (!dfa.accept[nextState] || nfa.accept[state] < dfa.accept[nextState])) {
					dfa.accept[nextState] = nfa.accept[state];
				}
			}
		}
	}

	delete[] closureMap;
	for (size_t i = 0; i < nfa.numStates; i++) {
		if (nfa.epsilon[i] != nullptr) {
			EpsTrans* node = nfa.epsilon[i];

			do {
				EpsTrans* next = node->next;
				delete node;
				node = next;
			} while (node != nullptr);
		}
	}
	delete[] nfa.epsilon;
	freeTables(nfa);

	return dfa;
}

size_t lexer::makeState(Tables& tables) {
	if (tables.numStates == tables.maxStates) expand(tables);

	return tables.numStates++;
}

void lexer::makeTransition(Tables& tables, size_t from, size_t to, unsigned char c) {
	if (c == (unsigned char)EOF && tables.epsilon != nullptr) {
		EpsTrans* newTransition = new EpsTrans{to, tables.epsilon[from]};

		tables.epsilon[from] = newTransition;
	} else {
		tables.next[from * 256 + c] = to;
	}
}

void lexer::makeAccept(Tables& tables, size_t state, size_t idx) {
	tables.accept[state] = idx;
}

void lexer::makeNFA(Tables& tables, parser::AST::Node* regex, size_t start, size_t end) {
	if (regex->type == "primitive::regex_literal") {
		string* str = dynamic_cast<parser::AST::RegexLiteralNode*>(regex)->str();

		size_t nextState = end;
		for (auto it = str->rbegin(); it != --str->rend(); it++) {
			size_t state = makeState(tables);

			makeTransition(tables, state, nextState, *it);
			nextState = state;
		}

		makeTransition(tables, start, nextState, str->front());
	} else if (regex->type == "primitive::regex_range") {
		string* chars = dynamic_cast<parser::AST::RegexRangeNode*>(regex)->chars();

		for (char c : *chars) {
			makeTransition(tables, start, end, c);
		}
	} else if (regex->type == "primitive::regex_or") {
		parser::AST::RegexOrNode* node = dynamic_cast<parser::AST::RegexOrNode*>(regex);

		for (auto rule : node->children()) {
			size_t subStart = makeState(tables), subEnd = makeState(tables);

			makeNFA(tables, rule, subStart, subEnd);

			makeTransition(tables, start, subStart, EOF);
			makeTransition(tables, subEnd, end, EOF);
		}
	} else if (regex->type == "primitive::regex_repeat") {
		parser::AST::RegexRepeatNode* node = dynamic_cast<parser::AST::RegexRepeatNode*>(regex);

		if (node->max == parser::AST::RegexRepeatNode::INFTY) {
			size_t ksStart = makeState(tables);

			if (node->min > 0) {
				size_t nextStart = start;

				for (int i = 0; i < node->min - 1; i++) {
					size_t nextEnd = makeState(tables);
					makeNFA(tables, node->children()[0], nextStart, nextEnd);
					nextStart = nextEnd;
				}

				makeNFA(tables, node->children()[0], nextStart, ksStart);
			} else {
				makeTransition(tables, start, ksStart, EOF);
			}

			size_t ksEnd = makeState(tables);
			makeTransition(tables, ksStart, ksEnd, EOF);

			size_t internalStart = makeState(tables), internalEnd = makeState(tables);
			makeTransition(tables, ksStart, internalStart, EOF);
			makeTransition(tables, internalEnd, ksEnd, EOF);
			makeTransition(tables, internalEnd, internalStart, EOF);

			makeNFA(tables, node->children()[0], internalStart, internalEnd);
			makeTransition(tables, ksEnd, end, EOF);
		} else {
			for (int times = node->min; times <= node->max; times++) {
				if (times == 0) {
					makeTransition(tables, start, end, EOF);
				} else {
					size_t nStart = makeState(tables), nEnd = makeState(tables);

					makeTransition(tables, start, nStart, EOF);
					makeTransition(tables, nEnd, end, EOF);

					size_t nextStart = nStart;
					for (int i = 0; i < times - 1; i++) {
						size_t nextEnd = makeState(tables);
						makeNFA(tables, node->children()[0], nextStart, nextEnd);
						nextStart = nextEnd;
					}

					makeNFA(tables, node->children()[0], nextStart, nEnd);
				}
			}
		}
	} else if (regex->type == "primitive::regex") {
		parser::AST::RegexNode* node = dynamic_cast<parser::AST::RegexNode*>(regex);
		size_t nextStart = start;

		for (auto child : node->children()) {
			size_t nextEnd = makeState(tables);
			makeNFA(tables, child, nextStart, nextEnd);
			nextStart = nextEnd;
		}

		makeTransition(tables, nextStart, end, EOF);
	} else if (regex->type == "primtive::pattern_ref") {
		throw domain_error("Table generation attempted before pattern dereferencing (or pattern reference not found)");
	}
}

set<size_t> lexer::findEpsClosure(const Tables& nfa, size_t state) {
	set<size_t> out;
	out.emplace(state);

	EpsTrans* node = nfa.epsilon[state];
	while (node != nullptr) {
		for (size_t nextState : findEpsClosure(nfa, node->to)) {
			out.emplace(nextState);
		}

		node = node->next;
	}

	return out;
}

void lexer::expand(Tables& tables) {
	if (tables.numStates == tables.maxStates) {
		size_t* next = new size_t[tables.maxStates * 2 * 256];
		size_t* accept = new size_t[tables.maxStates * 2];
		EpsTrans** epsilon = tables.epsilon != nullptr ? new EpsTrans*[tables.maxStates * 2] : nullptr;

		bzero(next, tables.maxStates * 2 * 256 * sizeof(size_t));
		bzero(accept, tables.maxStates * 2 * sizeof(size_t));
		if (epsilon != nullptr) {
			bzero(epsilon, tables.maxStates * 2 * sizeof(EpsTrans*));
		}

		for (size_t i = 0; i < tables.maxStates * 256; i++) {
			next[i] = tables.next[i];
		}

		for (size_t i = 0; i < tables.maxStates; i++) {
			accept[i] = tables.accept[i];
		}

		if (epsilon != nullptr) {
			for (size_t i = 0; i < tables.maxStates; i++) {
				epsilon[i] = tables.epsilon[i];
			}
		}

		freeTables(tables);
		if (tables.epsilon != nullptr) {
			delete[] tables.epsilon;
		}

		tables.next = next;
		tables.accept = accept;
		tables.epsilon = epsilon;
		tables.maxStates *= 2;
	}
}

void lexer::expand(set<size_t>*& map, size_t& size, size_t target) {
	set<size_t>* newMap = new set<size_t>[target];

	for (size_t i = 0; i < size; i++) {
		newMap[i] = map[i];
	}

	delete[] map;
	map = newMap;
	size = target;
}

void lexer::freeTables(const Tables& tables) {
	delete[] tables.next;
	delete[] tables.accept;
}

vector<parser::AST::TokenNode*> lexer::initPrimitives() {
	using namespace parser;

	vector<AST::TokenNode*> rules;

	rules.push_back(new AST::TokenNode("primitive::pattern", new AST::RegexNode({new AST::RegexLiteralNode(new string("!!!P"))})));
	rules.push_back(new AST::TokenNode("primitive::token", new AST::RegexNode({new AST::RegexLiteralNode(new string("!!!T"))})));
	string *every = new string(), *space = new string();
	// again, ignore eof
	for (unsigned char c = 0; c < 255; c++) {
		every->push_back(c);
		if (isspace(c)) {
			space->push_back(c);
		}
	}
	rules.push_back(new AST::TokenNode("primitive::ws", new AST::RegexNode({new AST::RegexRangeNode(space)})));
	rules.push_back(new AST::TokenNode("raw", new AST::RegexNode({new AST::RegexRangeNode(every)})));

	return rules;
}