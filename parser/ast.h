#ifndef PARSER_AST_H
#define PARSER_AST_H

#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace parser {
namespace AST {
class Node {
public:
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

	virtual std::ostream& _print(std::ostream& os, unsigned int level) const = 0;
};

class LeafNode : public Node {
public:
	LeafNode(const std::string& type) : Node(type), _data(nullptr) {}
	LeafNode(const std::string& type, void* data) : Node(type), _data(data) {}

protected:
	void* _data;

	virtual std::ostream& _print(std::ostream& os, unsigned int level) const override;
};

class InternalNode : public Node {
public:
	InternalNode(const std::string& type) : Node(type) {}
	InternalNode(const std::string& type, const std::vector<Node*> children) : Node(type), _children(children) {}

	std::vector<Node*> children() const { return _children; }

	InternalNode& append(Node* child);

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

class RegexLiteralNode : public LeafNode {
public:
	RegexLiteralNode(std::string* raw) : LeafNode("primitive::regex_literal", raw) {}

	std::string* str() const { return reinterpret_cast<std::string*>(_data); }

protected:
	virtual std::ostream& _print(std::ostream& os, unsigned int level) const override;
};

class RegexRangeNode : public LeafNode {
public:
	RegexRangeNode(std::string* raw) : LeafNode("primitive::regex_range", raw) {}

	std::string* chars() const { return reinterpret_cast<std::string*>(_data); }

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

class PatternNode : public InternalNode {
public:
	PatternNode(const std::string& name, RegexNode* pattern) : InternalNode("primitive::pattern", {pattern}), _name(name) {}

	std::string name() const { return _name; }

private:
	std::string _name;
};
}  // namespace AST
}  // namespace parser

#endif
