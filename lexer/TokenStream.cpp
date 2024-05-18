#include "TokenStream.h"

using namespace lexer;
using namespace std;

unsigned int lexer::line = 1;
unsigned int lexer::col = 1;

TokenStream::TokenStream(istream& in) : _dirty(false), _stream(in) {
	// _tokens.emplace("primitive::pattern", Pattern::bbb('P'));
	// _tokens.emplace("primitive::token", Pattern::bbb('T'));

	_tokenOrder.push_back("primitive::pattern");
	_tokenOrder.push_back("primitive::token");
}

TokenStream& TokenStream::operator>>(Token& tok) {
	if (_dirty) {
		tok = _currTok;
		_dirty = false;
	} else {
		_read();

		tok = _currTok;
		_dirty = false;
	}

	return *this;
}

Token TokenStream::peek(bool raw) {
	if (_dirty) {
		return _currTok;
	} else {
		_read(raw);

		return _currTok;
	}
}

Token TokenStream::read(bool raw) {
	if (_dirty) {
		_dirty = false;
		return _currTok;
	} else {
		_read(raw);
		_dirty = false;

		return _currTok;
	}
}

void TokenStream::_read(bool raw) {
	_currTok = Token{"", ""};

	if (raw) {
		char c = _stream.get();
		_currTok.type = "raw";
		_currTok.raw += c;

		if (c == '\n') {
			line++;
			col = 0;
		} else {
			col++;
		}

		_dirty = true;
	} else {
		while (!_stream.fail()) {
			char c = _stream.peek();

			_currTok.raw += c;
			_stream.get();

			if (_currTok.raw == "!!!P") {
				_currTok.type = "primitive::pattern";
				break;
			}

			if (c == '\n') {
				line++;
				col = 0;
			} else {
				col++;
			}
		}

		if (_currTok.type == "") {
			_currTok.type = "raw";
		}

		_dirty = true;
	}
}

TokenStream::operator bool() const {
	return !_stream.fail();
}
