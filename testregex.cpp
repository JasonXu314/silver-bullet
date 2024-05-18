#include "lexer.h"
#include "parser/utils.h"

using namespace std;
using namespace lexer;
using namespace parser;

int main() {
	TokenStream tokens(cin);

	AST::PatternNode* pattern = parsePattern(tokens);

	cout << *pattern << endl;

	return 0;
}