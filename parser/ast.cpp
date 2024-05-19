#include "ast.h"

#include "utils.h"

using namespace std;
using namespace parser;

AST::InternalNode& AST::InternalNode::append(Node* child) {
	_children.push_back(child);

	return *this;
}

ostream& AST::LeafNode::_print(std::ostream& os, unsigned int level) const {
	return indent(os, level) << "LEAF: " << type;
}

ostream& AST::InternalNode::_print(ostream& os, unsigned int level) const {
	indent(os, level) << "PARENT: " << type << endl;

	for (size_t i = 0; i < _children.size(); i++) {
		auto child = _children[i];
		child->_print(os, level + 1);
		if (i < _children.size() - 1) os << "\n";
	}

	return os;
}

ostream& AST::RegexNode::_print(ostream& os, unsigned int level) const {
	indent(os, level) << "REGEX:" << endl;

	for (size_t i = 0; i < _children.size(); i++) {
		auto child = _children[i];
		child->_print(os, level + 1);
		if (i < _children.size() - 1) os << "\n";
	}

	return os;
}

ostream& AST::RegexLiteralNode::_print(std::ostream& os, unsigned int level) const {
	return indent(os, level) << "REGEX LITERAL: " << *(reinterpret_cast<string*>(_data));
}

ostream& AST::RegexRangeNode::_print(std::ostream& os, unsigned int level) const {
	return indent(os, level) << "REGEX RANGE: " << *(reinterpret_cast<string*>(_data));
}

ostream& AST::RegexRepeatNode::_print(ostream& os, unsigned int level) const {
	indent(os, level) << "REGEX REPEAT: [" << min << ", " << max << "]" << endl;

	for (size_t i = 0; i < _children.size(); i++) {
		auto child = _children[i];
		child->_print(os, level + 1);
		if (i < _children.size() - 1) os << "\n";
	}

	return os;
}