#include "TokenStream.h"

using namespace lexer;
using namespace std;

unsigned int lexer::line = 1;
unsigned int lexer::col = 1;

TokenStream& TokenStream::operator>>(Token& tok) {
	if (_live) {
		if (_dirty) {
			tok = _currTok;
			_dirty = false;
		} else {
			_read();

			tok = _currTok;
			_dirty = false;
		}
	} else {
		if (_dirty) {
			tok = _currTok;
			_dirty = false;
		} else {
			tok.raw = "";
			tok.type = "error";
		}
	}

	return *this;
}

Token TokenStream::peek(bool raw) {
	if (_live) {
		if (_dirty) {
			return _currTok;
		} else {
			_read(raw);

			return _currTok;
		}
	} else {
		if (_dirty) {
			return _currTok;
		} else {
			return Token{"error", ""};
		}
	}
}

Token TokenStream::read(bool raw) {
	if (_live) {
		if (_dirty) {
			_dirty = false;
			return _currTok;
		} else {
			_read(raw);
			_dirty = false;

			return _currTok;
		}
	} else {
		if (_dirty) {
			_dirty = false;
			return _currTok;
		} else {
			return Token{"error", ""};
		}
	}
}

TokenStream::~TokenStream() {
	freeTables(_tables);
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
		stack<size_t> trail;
		size_t state = 1, pos = 0;

		while (true) {
			int ic = cin.peek();
			unsigned char c = ic;

			size_t nextState = _tables.next[state * 256 + c];

			if (ic == EOF || nextState == 0 || _hopeless.count({nextState, pos + 1})) {
				if (ic == EOF) {
					if (_currTok.raw.empty()) {
						_live = false;
						break;
					}
					cin.clear();
				}

				bool putback = false;
				while (!_tables.accept[state] && !trail.empty()) {
					putback = true;
					cin.putback(_currTok.raw.back());
					_currTok.raw.pop_back();
					_hopeless.emplace(state, pos);
					state = trail.top();
					trail.pop();
					pos--;
				}

				if (!putback && ic == EOF) _live = false;

				if (_tables.accept[state]) {
					_currTok.type = _tokenOrder[_tables.accept[state] - 1];
					state = 1;
					pos = 0;

					set<TrapRecord> newHopeless;
					for (auto record : _hopeless) {
						if (record.pos >= _currTok.raw.length()) {
							newHopeless.emplace(record.state, record.pos - _currTok.raw.length());
						}
					}
					_hopeless = newHopeless;

					break;
				} else {
					_currTok.type = "raw";
					state = 1;
					pos = 0;

					set<TrapRecord> newHopeless;
					for (auto record : _hopeless) {
						if (record.pos >= _currTok.raw.length()) {
							newHopeless.emplace(record.state, record.pos - _currTok.raw.length());
						}
					}
					_hopeless = newHopeless;

					break;
				}
			} else {
				_currTok.raw.push_back(c);
				trail.push(state);
				state = nextState;
				pos++;
				cin.get();
			}
		}

		_dirty = true;
	}
}

TokenStream::operator bool() const {
	return _live;
}

void TokenStream::updateTables(const Tables& tables, const vector<string>& names) {
	freeTables(_tables);
	_tables = tables;
	_tokenOrder = names;
}

bool lexer::operator<(const TrapRecord a, const TrapRecord b) {
	return a.state < b.state || a.pos < b.pos;
}