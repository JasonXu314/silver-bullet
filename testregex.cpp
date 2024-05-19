#include "lexer.h"
#include "parser/utils.h"

using namespace std;
using namespace lexer;
using namespace parser;

void deref(AST::InternalNode* pattern, const vector<AST::PatternNode*>& patterns);

int main() {
	TokenStream tokens(cin);
	vector<AST::PatternNode*> patterns;

	while (tokens) {
		AST::PatternNode* pattern = parsePattern(tokens);
		patterns.push_back(pattern);

		deref(pattern, patterns);

		cout << pattern->name() << endl;
		cout << *pattern << endl;
	}

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