#ifndef GULC_CONT_HPP
#define GULC_CONT_HPP

#include "Node.hpp"

namespace gulc {
    // Contract
    class Cont : public Node {
    public:
        static bool classof(const Node* node) { return node->getNodeKind() == Node::Kind::Cont; }

        enum class Kind {
            Requires,
            Ensures,
            Throws,
            Where
        };

        Cont::Kind getContKind() const { return _contKind; }
        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }
        virtual Cont* deepCopy() const = 0;

    protected:
        Cont::Kind _contKind;
        TextPosition _startPosition;
        TextPosition _endPosition;

        Cont(Cont::Kind contKind, TextPosition startPosition, TextPosition endPosition)
                : Node(Node::Kind::Cont), _contKind(contKind),
                  _startPosition(startPosition), _endPosition(endPosition) {}

    };
}

#endif //GULC_CONT_HPP
