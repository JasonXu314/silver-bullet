#include "lexer.h"
#include "lexer/utils.h"
#include "parser/utils.h"

using namespace std;
using namespace lexer;
using namespace parser;

int main() {
	TokenStream tokens(cin);

	AST::PatternNode* pattern = parsePattern(tokens);

	lexer::Tables tables = lexer::generateTables({pattern});

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

	cout << "accepting: ";
	for (size_t state = 0; state < tables.numStates; state++) {
		if (tables.accept[state]) {
			cout << state << " => " << pattern->name() << ", ";
		}
	}
	cout << endl;

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

	// cout << endl;

	return 0;
}