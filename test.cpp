#include <fstream>
#include <iostream>
#include <string>

#include "lexer.h"
#include "lexer/utils.h"

using namespace std;
using namespace lexer;

int main() {
	Tables tables = initPrimitives();

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

	freeTables(tables);

	return 0;
}