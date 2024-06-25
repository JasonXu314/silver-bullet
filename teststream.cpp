#include <fstream>
#include <iostream>
#include <string>

#include "lexer.h"
#include "lexer/utils.h"
#include "parser/ast.h"
#include "parser/utils.h"

using namespace std;
using namespace lexer;
using namespace parser;

string fix(string str);

AST::Node* deref(AST::Node* node, const vector<AST::PatternNode*>& patterns);

Tables makeTables(vector<AST::TokenNode*> rules, const vector<AST::PatternNode*>& patterns);

int main() {
	vector<AST::TokenNode*> rules = initPrimitives();
	vector<AST::PatternNode*> patterns;

	vector<string> names;
	names.push_back("primitive::pattern");
	names.push_back("primitive::token");
	names.push_back("primitive::ws");
	names.push_back("raw");

	TokenStream tokens(cin, makeTables(rules, patterns), names);

	while (tokens) {
		if (tokens.peek().type == "primitive::token") {
			AST::TokenNode* tokNode = parseToken(tokens);

			rules.insert(rules.end() - 1, tokNode);
			names.insert(names.end() - 1, tokNode->name());

			tokens.updateTables(makeTables(rules, patterns), names);
		} else if (tokens.peek().type == "primitive::pattern") {
			AST::PatternNode* patNode = parsePattern(tokens);

			patterns.push_back(patNode);
		} else {
			Token tok = tokens.peek();

			if (tok.type != "primitive::ws") cout << tok.type << ": " << fix(tok.raw) << "$" << endl;
			tokens.read();
		}
	}

	for (size_t i = 0; i < rules.size(); i++) {
		delete rules[i];
	}

	for (size_t i = 0; i < patterns.size(); i++) {
		delete patterns[i];
	}

	return 0;
}

string fix(string str) {
	for (size_t i = 0; i < str.length(); i++) {
		switch (str[i]) {
			case '\n':
				str[i] = '\\';
				str.insert(str.begin() + i + 1, 'n');
				break;
			case '\r':
				str[i] = '\\';
				str.insert(str.begin() + i + 1, 'r');
				break;
			case '\t':
				str[i] = '\\';
				str.insert(str.begin() + i + 1, 't');
				break;
		}
	}

	return str;
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
		} else if (node->type == "primitive::token") {
			AST::TokenNode* n = node->as<AST::TokenNode>();

			return new AST::TokenNode(n->name(), deref(n->children()[0], patterns)->as<AST::RegexNode>());
		} else {
			throw domain_error("Invalid regex node");
		}
	}
}

Tables makeTables(vector<AST::TokenNode*> rules, const vector<AST::PatternNode*>& patterns) {
	for (size_t i = 0; i < rules.size(); i++) {
		rules[i] = deref(rules[i], patterns)->as<AST::TokenNode>();
	}

	Tables tables = generateTables(rules);

	for (size_t i = 0; i < rules.size(); i++) {
		delete rules[i];
	}

	return tables;
}