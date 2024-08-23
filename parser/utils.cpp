#include "utils.h"

using namespace parser;
using namespace std;

AST::PatternNode* parser::parsePatternDecl(lexer::TokenStream& tokens) {
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

AST::TokenNode* parser::parseTokenDecl(lexer::TokenStream& tokens) {
	lexer::Token token = tokens.peek();

	if (token.type == "primitive::token") {
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

						return new AST::TokenNode(name, regex);
					} else {
						throw domain_error("Expected newline at end of pattern declaration, got '" + token.raw + "' (ln: " + to_string(lexer::line) +
										   ", col: " + to_string(lexer::col) + ")");
					}
				} else {
					throw domain_error("Expected whitespace");
				}
			} else {
				throw domain_error("Expected token name");
			}
		} else {
			throw domain_error("Expected whitespace");
		}
	} else {
		throw domain_error("Expected '!!!T', got '" + token.raw + "'");
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

						right->children()[0]->as<AST::RegexNode>()->children().clear();
						delete right;
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

AST::RuleNode* parser::parseRuleDecl(lexer::TokenStream& tokens) {
	lexer::Token token = tokens.peek();
	bool exact = false;

	if (token.type != "primitive::rule") throw domain_error("Expected '!!!R', got '" + token.raw + "'");

	tokens.read();

	token = tokens.peek(true);
	if (!(isspace(token.raw[0]) && token.raw[0] != '\n')) throw domain_error("Expected whitespace");

	do {
		tokens.read(true);
		token = tokens.peek(true);
	} while (isspace(token.raw[0]) && token.raw[0] != '\n');

	if (!(isalpha(token.raw[0]) || token.raw[0] == '_')) throw domain_error("Expected pattern name");

	string name;

	do {
		tokens.read(true);
		name += token.raw[0];
		token = tokens.peek(true);
	} while (isalnum(token.raw[0]) || token.raw[0] == '_');

	if (!(isspace(token.raw[0]) && token.raw[0] != '\n')) throw domain_error("Expected whitespace");

	do {
		tokens.read(true);
		token = tokens.peek(true);
	} while (isspace(token.raw[0]) && token.raw[0] != '\n');

	if (token.raw[0] != '=') throw domain_error("Expected '=>'");

	tokens.read(true);
	token = tokens.peek(true);

	if (token.raw[0] != '>') throw domain_error("Expected '=>' or '=>>'");

	tokens.read(true);
	token = tokens.peek(true);

	if (token.raw[0] == '>') {
		exact = true;

		tokens.read(true);
		token = tokens.peek(true);
	}

	if (!(isspace(token.raw[0]) && token.raw[0] != '\n')) throw domain_error("Expected whitespace");

	do {
		tokens.read(true);
		token = tokens.peek(true);
	} while (isspace(token.raw[0]) && token.raw[0] != '\n');

	vector<AST::Node*> bodies;

	do {
		if (token.raw[0] == '|') {
			do {
				tokens.read(true);
				token = tokens.peek(true);
			} while (isspace(token.raw[0]) && token.raw[0] != '\n');
		}

		AST::BodyNode* body = parseRuleBody(tokens, exact);
		bodies.push_back(body);

		token = tokens.peek(true);
		// no need to remove whitespace because parseRuleBody consumes until either '|' or ';'
	} while (token.raw[0] != ';');

	tokens.read(true);
	token = tokens.peek(true);
	while (isspace(token.raw[0]) && token.raw[0] != '\n') {
		tokens.read(true);
		token = tokens.peek(true);
	}

	if (token.raw[0] == '\n') {
		tokens.read(true);

		return new AST::RuleNode(name, bodies);
	} else {
		throw domain_error("Expected newline at end of rule declaration, got '" + token.raw + "' (ln: " + to_string(lexer::line) +
						   ", col: " + to_string(lexer::col) + ")");
	}
}

AST::BodyNode* parser::parseRuleBody(lexer::TokenStream& tokens, char end, bool exact) {
	lexer::Token token = tokens.peek(true);

	vector<AST::Node*> parts;

	do {
		if (token.raw[0] == '\'') {
			string literal;

			tokens.read(true);
			token = tokens.peek(true);

			while (token.raw[0] != '\'') {
				if (token.raw[0] == '\\') {
					tokens.read(true);
					token = tokens.peek(true);

					switch (token.raw[0]) {
						case 'n':
							literal += '\n';
							break;
						case 'r':
							literal += '\r';
							break;
						case '0':
							literal += '\0';
							break;
						case 't':
							literal += '\t';
							break;
						case '\\':
							literal += '\\';
							break;
						case '\'':
							literal += '\'';
							break;
					}
				} else {
					literal += token.raw[0];
				}

				tokens.read(true);
				token = tokens.peek(true);
			}

			tokens.read(true);
			token = tokens.peek(true);

			parts.push_back(new AST::RuleLiteralNode(new string(literal)));
		} else if (token.raw[0] == 't') {
			string rest = "oken::";

			for (size_t i = 0; i < rest.size(); i++) {
				tokens.read(true);
				token = tokens.peek(true);

				if (token.raw[0] != rest[i]) {
					throw domain_error(string("Expected '") + rest[i] + "', part of `token::` for token reference, got '" + token.raw + "'");
				}
			}

			tokens.read(true);
			token = tokens.peek(true);

			string* refName = new string();
			while (isalpha(token.raw[0]) || token.raw[0] == ':' || token.raw[0] == '_') {
				refName->push_back(token.raw[0]);

				tokens.read(true);
				token = tokens.peek(true);
			}

			parts.push_back(new AST::RuleTokenRefNode(refName));
		} else if (token.raw[0] == 'r') {
			string rest = "ule::";

			for (size_t i = 0; i < rest.size(); i++) {
				tokens.read(true);
				token = tokens.peek(true);

				if (token.raw[0] != rest[i]) {
					throw domain_error("Expected `rule::` for rule reference");
				}
			}

			tokens.read(true);
			token = tokens.peek(true);

			string* refName = new string();
			while (isalpha(token.raw[0]) || token.raw[0] == ':' || token.raw[0] == '_') {
				refName->push_back(token.raw[0]);

				tokens.read(true);
				token = tokens.peek(true);
			}

			parts.push_back(new AST::RuleRuleRefNode(refName));
		} else if (token.raw[0] == '{') {
			tokens.read(true);
			token = tokens.peek(true);

			while (isspace(token.raw[0]) && token.raw[0] != '\n') {
				tokens.read(true);
				token = tokens.peek(true);
			}

			AST::BodyNode* body = parseRuleBody(tokens, '}', exact);

			parts.push_back(new AST::RuleRepeatNode(body));
			token = tokens.peek(true);

			if (token.raw[0] == '}') {
				tokens.read(true);
				token = tokens.peek(true);
			} else {
				throw domain_error("Expected '}' after rule repeat");
			}
		} else if (token.raw[0] == '[') {
			tokens.read(true);
			token = tokens.peek(true);

			while (isspace(token.raw[0]) && token.raw[0] != '\n') {
				tokens.read(true);
				token = tokens.peek(true);
			}

			AST::BodyNode* body = parseRuleBody(tokens, ']', exact);

			parts.push_back(new AST::RuleOptionalNode(body));
			token = tokens.peek(true);

			if (token.raw[0] == ']') {
				tokens.read(true);
				token = tokens.peek(true);
			} else {
				throw domain_error("Expected ']' after rule optional");
			}
		} else if (end != '\0' && token.raw[0] != end) {
			if (end != '\0') {
				throw domain_error("Expected end of repetition/optional with '" + string(1, end) + "', got '" + token.raw + "'");
			} else {
				throw domain_error("Expected rule literal or token/rule reference or repetition or optional, got '" + token.raw + "'");
			}
		}

		while (isspace(token.raw[0])) {
			tokens.read(true);
			token = tokens.peek(true);
		}
	} while (token.raw[0] != '|' && token.raw[0] != ';' && (end == '\0' || token.raw[0] != end));

	return new AST::BodyNode(parts, exact);
}

AST::InternalNode* parser::parse(lexer::TokenStream& tokens, AST::RuleNode* rule, const map<string, AST::RuleNode*>& rules) {
	for (auto child : rule->children()) {
		try {
			// cout << "parsing as: " << rule->name() << endl;
			return parse(tokens, rule->name(), child->as<AST::BodyNode>(), rules);
		} catch (exception& e) {
			// cout << "Got error: " << e.what() << endl;
		}
	}

	throw domain_error("Expected production `" + rule->name() + "`");
}

AST::InternalNode* parser::parse(lexer::TokenStream& tokens, const string& name, AST::BodyNode* body, const map<string, AST::RuleNode*>& rules) {
	lexer::Token token;
	stack<lexer::Token> consumed;

	vector<AST::Node*> children;

	if (!body->children().empty() &&
		(body->children()[0]->type != "primitive::rule_token_ref" || *body->children()[0]->as<AST::RuleTokenRefNode>()->name() != "primitive::ws")) {
		while (tokens.peek().type == "primitive::ws") tokens.read();
	}

	for (size_t i = 0; i < body->children().size(); i++) {
		auto part = body->children()[i];

		if (part->type == "primitive::rule_literal") {
			// cout << "part literal" << endl;
			AST::RuleLiteralNode* literal = part->as<AST::RuleLiteralNode>();

			for (char c : *literal->str()) {
				token = tokens.read(true);

				if (c != token.raw[0]) {
					while (!consumed.empty()) {
						tokens.putback(consumed.top());
						consumed.pop();
					}

					tokens.pack();
					throw domain_error("Expected literal `" + *literal->str() + "`");
				}

				consumed.push(token);
			}

			children.push_back(new AST::LeafNode<string>("primitive::literal", new string(*literal->str())));
		} else if (part->type == "primitive::rule_token_ref") {
			AST::RuleTokenRefNode* ref = part->as<AST::RuleTokenRefNode>();
			// cout << "part tok ref: " << *ref->name() << endl;

			token = tokens.read();
			consumed.push(token);

			if (token.type != *ref->name()) {
				while (!consumed.empty()) {
					tokens.putback(consumed.top());
					consumed.pop();
				}

				tokens.pack();
				throw domain_error("Expected token `" + *ref->name() + "`, got `" + token.type + "`: '" + token.raw + "'");
			}
			// else {
			// 	cout << "read tok " << token.type << ": " << token.raw << endl;
			// }

			children.push_back(new AST::LeafNode<string>(token.type, new string(token.raw)));
		} else if (part->type == "primitive::rule_rule_ref") {
			AST::RuleRuleRefNode* ref = part->as<AST::RuleRuleRefNode>();
			// cout << "part rule ref: " << *ref->name() << endl;

			if (rules.count(*ref->name())) {
				children.push_back(parse(tokens, rules.at(*ref->name()), rules));
			} else {
				while (!consumed.empty()) {
					tokens.putback(consumed.top());
					consumed.pop();
				}

				tokens.pack();
				throw domain_error("Unknown production `" + *ref->name() + "`");
			}
		} else if (part->type == "primitive::rule_repeat") {
			AST::BodyNode* body = part->as<AST::RuleRepeatNode>()->children()[0]->as<AST::BodyNode>();
			set<string> FIRST = utils::findFIRSTSet(body, rules);
			token = tokens.peek();
			// cout << "part repeat" << endl;

			while (token.type == "raw" ? FIRST.count(token.raw) : FIRST.count("token::" + token.type)) {
				AST::InternalNode* temp = parse(tokens, "", body, rules)->as<AST::InternalNode>();

				for (auto child : temp->children()) {
					children.push_back(child);
				}

				token = tokens.peek();
				temp->children().clear();
				delete temp;
			}
		} else if (part->type == "primitive::rule_optional") {
			AST::BodyNode* body = part->as<AST::RuleRepeatNode>()->children()[0]->as<AST::BodyNode>();
			set<string> FIRST = utils::findFIRSTSet(body, rules);
			token = tokens.peek();
			// cout << "part optional" << endl;

			if (token.type == "raw" ? FIRST.count(token.raw) : FIRST.count("token::" + token.type)) {
				// cout << "found: " << token.type << endl;
				AST::InternalNode* temp = parse(tokens, "", body, rules)->as<AST::InternalNode>();

				for (auto child : temp->children()) {
					children.push_back(child);
				}

				temp->children().clear();
				delete temp;
			}
			// else {
			// 	cout << "not found: " << token.type << endl;
			// }
		}

		if (i + 1 < body->children().size() && (body->children()[i + 1]->type != "primitive::rule_token_ref" ||
												*body->children()[i + 1]->as<AST::RuleTokenRefNode>()->name() != "primitive::ws")) {
			while (tokens.peek().type == "primitive::ws") tokens.read();
		}
	}

	// TODO: add follow set computation so that we can know whether to consume whitespace here

	return new AST::InternalNode(name, children);
}

set<string> parser::utils::findFIRSTSet(AST::Node* node, const map<string, AST::RuleNode*>& rules) {
	if (node->type == "primitive::rule") {
		set<string> FIRST;

		for (auto child : node->as<AST::RuleNode>()->children()) {
			for (auto first : findFIRSTSet(child, rules)) {
				FIRST.emplace(first);
			}
		}

		return FIRST;
	} else if (node->type == "primitive::body") {
		set<string> FIRST;
		AST::BodyNode* body = node->as<AST::BodyNode>();
		vector<AST::Node*>::iterator it;

		for (it = body->children().begin();
			 it != body->children().end() && ((*it)->type == "primitive::rule_repeat" || (*it)->type == "primitive::rule_optional"); it++) {
			for (auto str : findFIRSTSet(*it, rules)) {
				FIRST.emplace(str);
			}
		}

		if (it != body->children().end()) {
			for (auto str : findFIRSTSet(*it, rules)) {
				FIRST.emplace(str);
			}
		}

		return FIRST;
	} else if (node->type == "primitive::rule_token_ref") {
		set<string> FIRST;
		FIRST.emplace("token::" + *node->as<AST::RuleTokenRefNode>()->name());

		return FIRST;
	} else if (node->type == "primitive::rule_rule_ref") {
		return findFIRSTSet(rules.at(*node->as<AST::RuleRuleRefNode>()->name()), rules);
	} else if (node->type == "primitive::rule_repeat") {
		return findFIRSTSet(node->as<AST::RuleRepeatNode>()->children()[0], rules);
	} else if (node->type == "primitive::rule_optional") {
		return findFIRSTSet(node->as<AST::RuleOptionalNode>()->children()[0], rules);
	} else if (node->type == "primitive::rule_literal") {
		set<string> FIRST;
		FIRST.emplace(string(1, (*node->as<AST::RuleLiteralNode>()->str())[0]));

		return FIRST;
	} else {
		return set<string>();
	}
}

ostream& indent(ostream& stream, unsigned int level) {
	return stream << setw(level * 4) << "";
}