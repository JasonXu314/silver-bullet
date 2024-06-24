#include <fstream>

#include "lexer.h"
#include "lexer/utils.h"
#include "parser/utils.h"

using namespace std;
using namespace lexer;
using namespace parser;

AST::Node* deref(AST::Node* node, const vector<AST::PatternNode*>& patterns);

int main() {
	TokenStream tokens(cin, generateTables(initPrimitives()));

	vector<AST::PatternNode*> patterns;

	while (tokens) {
		patterns.push_back(parsePattern(tokens));
	}

	vector<AST::PatternNode*> derefPatterns;

	for (auto pattern : patterns) {
		derefPatterns.push_back(deref(pattern, patterns)->as<AST::PatternNode>());
	}

	lexer::Tables tables = lexer::generateTables(derefPatterns);

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

	for (auto pattern : patterns) {
		delete pattern;
	}

	for (auto pattern : derefPatterns) {
		delete pattern;
	}

	freeTables(tables);

	return 0;
}

AST::Node* deref(AST::Node* node, const vector<AST::PatternNode*>& patterns) {
	if (node->type == "primitive::regex_literal") {
		return new AST::RegexLiteralNode(node->as<AST::RegexLiteralNode>());
	} else if (node->type == "primitive::regex_range") {
		return new AST::RegexRangeNode(node->as<AST::RegexRangeNode>());
	} else if (node->type == "primitive::pattern_ref") {
		string name = *node->as<AST::RegexPatternRefNode>()->name();
		AST::PatternNode* ref = nullptr;

		for (auto pattern : patterns) {
			if (pattern->name() == name) {
				ref = pattern;
				break;
			}
		}

		if (ref != nullptr) {
			return deref(ref->children()[0], patterns);
		} else {
			return node;
		}
	} else {
		if (node->type == "primitive::regex") {
			vector<AST::Node*> children;

			for (auto child : node->as<AST::RegexNode>()->children()) {
				children.push_back(deref(child, patterns));
			}

			return new AST::RegexNode(children);
		} else if (node->type == "primitive::regex_or") {
			vector<AST::Node*> children;

			for (auto child : node->as<AST::RegexOrNode>()->children()) {
				children.push_back(deref(child, patterns));
			}

			return new AST::RegexOrNode(children);
		} else if (node->type == "primitive::regex_repeat") {
			AST::RegexRepeatNode* n = node->as<AST::RegexRepeatNode>();

			return new AST::RegexRepeatNode(n->min, n->max, deref(n->children()[0], patterns));
		} else if (node->type == "primitive::pattern") {
			AST::PatternNode* n = node->as<AST::PatternNode>();

			return new AST::PatternNode(n->name(), deref(n->children()[0], patterns)->as<AST::RegexNode>());
		} else {
			throw domain_error("Invalid regex node");
		}
	}
}