#include <fstream>
#include <iostream>
#include <string>

#include "lexer.h"
#include "lexer/utils.h"

using namespace std;
using namespace lexer;

string fix(string str);

int main() {
	TokenStream tokens(cin);
	Token tok;

	while (tokens >> tok) {
		cout << tok.type << ": " << fix(tok.raw) << "$" << endl;
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