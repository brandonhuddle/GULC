#ifndef GULC_IDENTIFIER_HPP
#define GULC_IDENTIFIER_HPP

#include <string>
#include "Node.hpp"

namespace gulc {
    class Identifier final : public Node {
    public:
        static bool classof(const Node* node) { return node->getNodeKind() == Node::Kind::Identifier; }

        Identifier()
                : Identifier({}, {}, "") {}

        Identifier(TextPosition startPositon, TextPosition endPosition, std::string name)
                : Node(Node::Kind::Identifier),
                  _startPosition(startPositon), _endPosition(endPosition), _name(std::move(name)) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }
        std::string const& name() const { return _name; }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;
        std::string _name;

    };
}

#endif //GULC_IDENTIFIER_HPP
