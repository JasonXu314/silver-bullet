#ifndef PARSER_AST_H
#define PARSER_AST_H

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

std::ostream& indent(std::ostream& stream, unsigned int level);

namespace parser {
namespace AST {
class Node {
public:
	template <class DT>
	friend class LeafNode;
	friend class InternalNode;
	friend class RegexNode;
	friend class RegexRepeatNode;

	const std::string type;

	virtual ~Node() = default;

	friend std::ostream& operator<<(std::ostream& os, const Node& node) { return node._print(os, 0); }
	friend std::ostream& operator<<(std::ostream& os, Node const* node) { return os << *node; }

	template <class T>
	T* as() {
		return reinterpret_cast<T*>(this);
	}

protected:
	Node() = delete;
	Node(const std::string& type) : type(type) {}
	Node(Node* other) : type(other->type) {}

	virtual std::ostream& _print(std::ostream& os, unsigned int level) const = 0;
};

template <class DT>
class LeafNode : public Node {
public:
	LeafNode(const std::string& type) : Node(type), _data(nullptr) {}
	LeafNode(const std::string& type, DT* data) : Node(type), _data(data) {}
	LeafNode(LeafNode* other) : Node(other), _data(new DT(*other->_data)) {}

	virtual ~LeafNode() { delete _data; }

protected:
	DT* _data;

	virtual std::ostream& _print(std::ostream& os, unsigned int level) const override { return indent(os, level) << "LEAF: " << type; }
};

class InternalNode : public Node {
public:
	InternalNode(const std::string& type) : Node(type) {}
	InternalNode(const std::string& type, const std::vector<Node*> children) : Node(type), _children(children) {}

	std::vector<Node*>& children() { return _children; }

	InternalNode& append(Node* child);

	virtual ~InternalNode();

protected:
	std::vector<Node*> _children;

	virtual std::ostream& _print(std::ostream& os, unsigned int level) const override;
};

class RegexNode : public InternalNode {
public:
	RegexNode(const std::vector<Node*>& children) : InternalNode("primitive::regex", children) {}

protected:
	virtual std::ostream& _print(std::ostream& os, unsigned int level) const override;
};

class RegexLiteralNode : public LeafNode<std::string> {
public:
	RegexLiteralNode(std::string* raw) : LeafNode("primitive::regex_literal", raw) {}
	RegexLiteralNode(RegexLiteralNode* other) : LeafNode<std::string>(other) {}

	std::string* str() const { return _data; }

protected:
	virtual std::ostream& _print(std::ostream& os, unsigned int level) const override;
};

class RegexRangeNode : public LeafNode<std::string> {
public:
	RegexRangeNode(std::string* raw) : LeafNode("primitive::regex_range", raw) {}
	RegexRangeNode(RegexRangeNode* other) : LeafNode<std::string>(other) {}

	std::string* chars() const { return _data; }

protected:
	virtual std::ostream& _print(std::ostream& os, unsigned int level) const override;
};

class RegexOrNode : public InternalNode {
public:
	RegexOrNode(const std::vector<Node*>& children) : InternalNode("primitive::regex_or", children) {}
};

class RegexRepeatNode : public InternalNode {
public:
	enum { INFTY = -1 };

	const int min, max;

	RegexRepeatNode(int min, int max, Node* pattern) : InternalNode("primitive::regex_repeat", {pattern}), min(min), max(max) {}

protected:
	virtual std::ostream& _print(std::ostream& os, unsigned int level) const override;
};

class RegexPatternRefNode : public LeafNode<std::string> {
public:
	RegexPatternRefNode(std::string* name) : LeafNode("primitive::pattern_ref", name) {}

	std::string* name() const { return _data; }
};

class PatternNode : public InternalNode {
public:
	PatternNode(const std::string& name, RegexNode* pattern) : InternalNode("primitive::pattern", {pattern}), _name(name) {}

	std::string name() const { return _name; }

private:
	std::string _name;
};

class TokenNode : public InternalNode {
public:
	TokenNode(const std::string& name, RegexNode* pattern) : InternalNode("primitive::token", {pattern}), _name(name) {}

	std::string name() const { return _name; }

private:
	std::string _name;
};
}  // namespace AST
}  // namespace parser

#endif
