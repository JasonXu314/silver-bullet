#ifndef LEXER_TOKEN_STREAM_H
#define LEXER_TOKEN_STREAM_H

#include <cctype>
#include <cmath>
#include <functional>
#include <iostream>
#include <map>
#include <stdexcept>
#include <string>
#include <vector>

#include "tokens.h"

namespace lexer {
extern unsigned int line, col;

class TokenStream {
public:
	TokenStream(std::istream& in);

	TokenStream& operator>>(Token& tok);

	Token peek(bool raw = false);

	Token read(bool raw = false);

	operator bool() const;

private:
	bool _dirty;
	std::istream& _stream;
	Token _currTok;
	std::vector<std::string> _patternOrder;
	std::vector<std::string> _tokenOrder;

	void _read(bool raw = false);
};
}  // namespace lexer

#endif