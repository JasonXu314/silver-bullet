#ifndef LEXER_TOKEN_STREAM_H
#define LEXER_TOKEN_STREAM_H

#include <cctype>
#include <cmath>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <stack>
#include <stdexcept>
#include <string>
#include <vector>

#include "tokens.h"
#include "utils.h"

namespace lexer {
extern unsigned int line, col;

struct TrapRecord {
	size_t state;
	size_t pos;
};

bool operator<(const lexer::TrapRecord a, const lexer::TrapRecord b);

class TokenStream {
public:
	TokenStream(std::istream& in, const Tables& initialTables = initPrimitives());

	TokenStream& operator>>(Token& tok);

	Token peek(bool raw = false);

	Token read(bool raw = false);

	operator bool() const;

private:
	bool _dirty;
	bool _live;
	std::istream& _stream;
	Token _currTok;
	std::vector<std::string> _patternOrder;
	std::vector<std::string> _tokenOrder;
	Tables _tables;
	std::set<TrapRecord> _hopeless;

	void _read(bool raw = false);
};
}  // namespace lexer

#endif