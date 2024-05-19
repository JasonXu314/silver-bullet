#include <fstream>

#include "lexer.h"
#include "lexer/utils.h"
#include "parser/utils.h"

using namespace std;
using namespace lexer;
using namespace parser;

void deref(AST::InternalNode* pattern, const vector<AST::PatternNode*>& patterns);

int main() {
	TokenStream tokens(cin);

	vector<AST::PatternNode*> patterns;

	while (tokens) {
		patterns.push_back(parsePattern(tokens));
	}

	for (auto pattern : patterns) {
		deref(pattern, patterns);
	}

	lexer::Tables tables = lexer::generateTables(patterns);

	cout << "     State\nChar ";
	for (size_t state = 0; state < tables.numStates; state++) {
		cout << left << setw(3) << state << ' ';
	}
	cout << endl;

	for (char c = ' '; c <= '~'; c++) {
		cout << right << setw(4) << c << ' ';

		for (size_t state = 0; state < tables.numStates; state++) {
			cout << left << setw(3) << tables.next[state * 256 + c] << ' ';
		}

		cout << endl;
	}

	// cout << right << setw(4) << "eps" << ' ';

	// bool hasLink = true;
	// for (size_t idx = 0; hasLink; idx++) {
	// 	hasLink = false;
	// 	for (size_t state = 0; state < tables.numStates; state++) {
	// 		lexer::EpsTrans* link = tables.epsilon[state];

	// 		for (size_t i = 0; i < idx && link != nullptr; i++) {
	// 			link = link->next;
	// 		}

	// 		if (link != nullptr) {
	// 			cout << left << setw(3) << link->to << ' ';
	// 			hasLink = true;
	// 		} else {
	// 			cout << "    ";
	// 		}
	// 	}

	// 	if (hasLink) {
	// 		cout << endl << "     ";
	// 	}
	// }

	cout << endl;

	cout << "accepting: ";
	for (size_t state = 0; state < tables.numStates; state++) {
		if (tables.accept[state]) {
			cout << state << " => " << patterns[tables.accept[state] - 1]->name() << ", ";
		}
	}
	cout << endl;

	ofstream dump("tables.dump");

	dump << "size_t next[" << tables.numStates * 256 << "] = {" << endl;
	for (size_t i = 0; i < tables.numStates * 256 - 1; i++) {
		dump << tables.next[i] << ", ";
	}
	dump << tables.next[tables.numStates * 256 - 1] << "\n};\n\nsize_t accept[" << tables.numStates << "] = {" << endl;
	for (size_t i = 0; i < tables.numStates - 1; i++) {
		dump << tables.accept[i] << ", ";
	}
	dump << tables.accept[tables.numStates - 1] << "\n};" << endl;

	dump.close();

	return 0;
}

void deref(AST::InternalNode* pattern, const vector<AST::PatternNode*>& patterns) {
	for (auto& child : pattern->children()) {
		if (child->type == "primitive::regex") {
			deref(child->as<AST::RegexNode>(), patterns);
		} else if (child->type == "primitive::regex_or") {
			deref(child->as<AST::RegexOrNode>(), patterns);
		} else if (child->type == "primitive::regex_repeat") {
			deref(child->as<AST::RegexRepeatNode>(), patterns);
		} else if (child->type == "primitive::pattern_ref") {
			string name = *child->as<AST::RegexPatternRefNode>()->name();

			for (auto pattern : patterns) {
				if (pattern->name() == name) {
					delete child;
					child = pattern->children()[0];
					break;
				}
			}
		}
	}
}