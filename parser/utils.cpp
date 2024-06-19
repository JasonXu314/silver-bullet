#include "utils.h"

using namespace parser;
using namespace std;

AST::PatternNode* parser::parsePattern(lexer::TokenStream& tokens) {
	lexer::Token token = tokens.peek();

	if (token.type == "primitive::pattern") {
		tokens.read();

		token = tokens.peek(true);
		if (isspace(token.raw[0]) && token.raw[0] != '\n') {
			do {
				tokens.read(true);
				token = tokens.peek(true);
			} while (isspace(token.raw[0]) && token.raw[0] != '\n');

			if (isalpha(token.raw[0]) || token.raw[0] == '_') {
				string name;

				do {
					tokens.read(true);
					name += token.raw[0];
					token = tokens.peek(true);
				} while (isalnum(token.raw[0]) || token.raw[0] == '_');

				if (isspace(token.raw[0]) && token.raw[0] != '\n') {
					do {
						tokens.read(true);
						token = tokens.peek(true);
					} while (isspace(token.raw[0]) && token.raw[0] != '\n');

					AST::RegexNode* regex = parseRegex(tokens);

					token = tokens.peek(true);
					while (isspace(token.raw[0]) && token.raw[0] != '\n') {
						tokens.read(true);
						token = tokens.peek(true);
					}

					if (token.raw[0] == '\n') {
						tokens.read(true);

						return new AST::PatternNode(name, regex);
					} else {
						throw domain_error("Expected newline at end of pattern declaration, got '" + token.raw + "' (ln: " + to_string(lexer::line) +
										   ", col: " + to_string(lexer::col) + ")");
					}
				} else {
					throw domain_error("Expected whitespace");
				}
			} else {
				throw domain_error("Expected pattern name");
			}
		} else {
			throw domain_error("Expected whitespace");
		}
	} else {
		throw domain_error("Expected '!!!P', got '" + token.raw + "'");
	}
}

AST::RegexNode* parser::parseRegex(lexer::TokenStream& tokens) {
	lexer::Token token = tokens.peek(true);

	if (token.type == "raw") {
		vector<AST::Node*> parts;
		AST::RegexLiteralNode* literal = nullptr;

		while (!isspace(token.raw[0])) {
			switch (token.raw[0]) {
				case '[': {
					if (literal != nullptr) {
						parts.push_back(literal);
						literal = nullptr;
					}

					parts.push_back(parseRange(tokens));
					token = tokens.peek(true);
					continue;
				}
				case '(': {
					if (literal != nullptr) {
						parts.push_back(literal);
						literal = nullptr;
					}

					tokens.read(true);

					parts.push_back(parseRegex(tokens));

					if (tokens.peek(true).raw[0] != ')') {
						throw domain_error("Expected ')' after group in regex");
					}

					tokens.read(true);
					token = tokens.peek(true);
					continue;
				}
				case '|': {
					if (literal != nullptr) {
						parts.push_back(literal);
						literal = nullptr;
					}

					tokens.read(true);
					tokens.peek(true);
					AST::RegexOrNode* orNode = new AST::RegexOrNode({new AST::RegexNode(parts)});

					AST::RegexNode* right = parseRegex(tokens);
					if (right->children()[0]->type == "primitive::regex_or") {
						for (auto child : right->children()[0]->as<AST::RegexOrNode>()->children()) {
							orNode->append(child);
						}
					} else {
						orNode->append(right);
					}

					return new AST::RegexNode({orNode});
				}
				case '?': {
					if (literal != nullptr) {
						parts.push_back(literal);
						literal = nullptr;
					} else if (parts.size() == 0) {
						throw domain_error("Unexpected '?' in regex pattern");
					}

					AST::Node* last = parts.back();
					parts.pop_back();

					if (last->type == "primitive::regex_literal") {
						char lastChar = last->as<AST::RegexLiteralNode>()->str()->back();
						last->as<AST::RegexLiteralNode>()->str()->pop_back();
						parts.push_back(last);

						string* charStr = new string();
						charStr->push_back(lastChar);
						parts.push_back(new AST::RegexRepeatNode(0, 1, new AST::RegexLiteralNode(charStr)));
					} else {
						parts.push_back(new AST::RegexRepeatNode(0, 1, last));
					}
					break;
				}
				case '+': {
					if (literal != nullptr) {
						parts.push_back(literal);
						literal = nullptr;
					} else if (parts.size() == 0) {
						throw domain_error("Unexpected '?' in regex pattern");
					}

					AST::Node* last = parts.back();
					parts.pop_back();

					if (last->type == "primitive::regex_literal") {
						char lastChar = last->as<AST::RegexLiteralNode>()->str()->back();
						last->as<AST::RegexLiteralNode>()->str()->pop_back();
						parts.push_back(last);

						string* charStr = new string();
						charStr->push_back(lastChar);
						parts.push_back(new AST::RegexRepeatNode(1, AST::RegexRepeatNode::INFTY, new AST::RegexLiteralNode(charStr)));
					} else {
						parts.push_back(new AST::RegexRepeatNode(1, AST::RegexRepeatNode::INFTY, last));
					}
					break;
				}
				case '*': {
					if (literal != nullptr) {
						parts.push_back(literal);
						literal = nullptr;
					} else if (parts.size() == 0) {
						throw domain_error("Unexpected '?' in regex pattern");
					}

					AST::Node* last = parts.back();
					parts.pop_back();

					if (last->type == "primitive::regex_literal") {
						char lastChar = last->as<AST::RegexLiteralNode>()->str()->back();
						last->as<AST::RegexLiteralNode>()->str()->pop_back();
						parts.push_back(last);

						string* charStr = new string();
						charStr->push_back(lastChar);
						parts.push_back(new AST::RegexRepeatNode(0, AST::RegexRepeatNode::INFTY, new AST::RegexLiteralNode(charStr)));
					} else {
						parts.push_back(new AST::RegexRepeatNode(0, AST::RegexRepeatNode::INFTY, last));
					}
					break;
				}
				case '{': {
					tokens.read(true);
					token = tokens.peek(true);

					if (isdigit(token.raw[0])) {
						if (literal != nullptr) {
							parts.push_back(literal);
							literal = nullptr;
						} else if (parts.size() == 0) {
							throw domain_error("Unexpected range repeat in regex pattern");
						}

						string min, max;

						do {
							tokens.read(true);
							min += token.raw;
							token = tokens.peek(true);
						} while (isdigit(token.raw[0]));

						if (token.raw[0] == ',') {
							tokens.read(true);
							token = tokens.peek(true);

							if (isdigit(token.raw[0])) {
								do {
									tokens.read(true);
									max += token.raw;
									token = tokens.peek(true);
								} while (isdigit(token.raw[0]));
							} else if (token.raw[0] == '}') {
								max = "infty";
							} else {
								throw domain_error("Expected integer or '}' after ',' in range repeat");
							}
						}

						AST::Node* last = parts.back();
						parts.pop_back();

						if (last->type == "primitive::regex_literal") {
							char lastChar = last->as<AST::RegexLiteralNode>()->str()->back();
							last->as<AST::RegexLiteralNode>()->str()->pop_back();
							parts.push_back(last);

							string* charStr = new string();
							charStr->push_back(lastChar);
							parts.push_back(new AST::RegexRepeatNode(stoi(min), max == "infty" ? AST::RegexRepeatNode::INFTY : stoi(max),
																	 new AST::RegexLiteralNode(charStr)));
						} else {
							parts.push_back(new AST::RegexRepeatNode(stoi(min), max == "infty" ? AST::RegexRepeatNode::INFTY : stoi(max), last));
						}
					}
					// add case here for x{,n} syntax
					else {
						if (literal != nullptr) {
							parts.push_back(literal);
							literal = nullptr;
						}

						string name;

						do {
							tokens.read(true);
							name += token.raw;
							token = tokens.peek(true);
						} while (isalnum(token.raw[0]) || token.raw[0] == '_');

						if (token.raw[0] != '}') {
							throw domain_error("Expected '}' after pattern ref name");
						}

						parts.push_back(new AST::RegexPatternRefNode(new string(name)));
					}
					break;
				}
				case ')': {
					if (literal != nullptr) {
						parts.push_back(literal);
					}

					return new AST::RegexNode(parts);
				}
				default: {
					if (token.raw[0] == '\\') {
						tokens.read(true);
						token = tokens.peek(true);

						switch (token.raw[0]) {
							case 'n':
								token.raw[0] = '\n';
								break;
							case 'r':
								token.raw[0] = '\r';
								break;
							case '0':
								token.raw[0] = '\0';
								break;
							case 't':
								token.raw[0] = '\t';
								break;
							case '\\':
								token.raw[0] = '\\';
								break;
							case ']':
							case '-':
							case '+':
							case '{':
							case '}':
							case '|':
							case '.':
							case ' ':
							case '(':
							case ')':
								break;
						}
					}

					if (literal == nullptr) {
						literal = new AST::RegexLiteralNode(new string(token.raw));
					} else {
						literal->str()->push_back(token.raw[0]);
					}
					break;
				}
			}

			tokens.read(true);
			token = tokens.peek(true);
		}

		if (literal != nullptr) {
			parts.push_back(literal);
		}

		return new AST::RegexNode(parts);
	} else {
		throw domain_error("Expected raw token in regex");
	}
}

AST::RegexRangeNode* parser::parseRange(lexer::TokenStream& tokens) {
	lexer::Token token = tokens.peek();

	if (token.type == "raw") {
		if (token.raw[0] == '[') {
			string chars;

			tokens.read(true);
			token = tokens.peek(true);

			while (token.raw[0] != ']') {
				if (token.raw[0] == '\\') {
					tokens.read(true);
					token = tokens.peek(true);

					switch (token.raw[0]) {
						case 'n':
							chars += '\n';
							break;
						case 'r':
							chars += '\r';
							break;
						case '0':
							chars += '\0';
							break;
						case 't':
							chars += '\t';
							break;
						case '\\':
							chars += '\\';
							break;
						case ']':
							chars += ']';
							break;
						case '-':
							chars += '-';
							break;
						case '+':
							chars += '+';
							break;
						case '{':
							chars += '{';
							break;
						case '}':
							chars += '}';
							break;
					}
				} else if (token.raw[0] == '-') {
					if (chars.size() == 0) {
						throw domain_error("Unexpected '-' in regex range");
					}

					tokens.read(true);
					token = tokens.peek(true);

					char start = chars.back();
					char end = token.raw[0];

					if (start >= end) {
						throw domain_error(string("Invalid regex range progression: '") + start + "' to '" + end + "'");
					}

					for (char c = start + 1; c <= end; c++) {
						chars.push_back(c);
					}
				} else {
					chars += token.raw[0];
				}

				tokens.read();
				token = tokens.peek(true);
			}

			if (token.raw[0] == ']') {
				tokens.read();

				return new AST::RegexRangeNode(new string(chars));
			} else {
				throw domain_error("Expected ']' to end regex range");
			}
		} else {
			throw domain_error("Expected '[' to begin regex range, got '" + token.raw + "' (ln: " + to_string(lexer::line) +
							   ", col: " + to_string(lexer::col) + ")");
		}
	} else {
		throw domain_error("Expected raw token in regex range");
	}
}

ostream& indent(ostream& stream, unsigned int level) {
	return stream << setw(level * 4) << "";
}