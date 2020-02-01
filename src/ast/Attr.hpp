#ifndef GULC_ATTR_HPP
#define GULC_ATTR_HPP

#include "Node.hpp"

namespace gulc {
    class Attr : public Node {
    public:
        static bool classof(const Node* node) { return node->getNodeKind() == Node::Kind::Attr; }

        enum class Kind {
            // References an `AttributeDecl`...
            Custom,
            // An unresolved attribute. Could be `pod` or something custom
            Unresolved,

            Pod
        };

        Attr::Kind getAttrKind() const { return _attrKind; }

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

    protected:
        Attr::Kind _attrKind;
        TextPosition _startPosition;
        TextPosition _endPosition;

        Attr(Attr::Kind attrKind, TextPosition startPosition, TextPosition endPosition)
                : Node(Node::Kind::Attr), _attrKind(attrKind),
                  _startPosition(startPosition), _endPosition(endPosition) {}

    };
}

#endif //GULC_ATTR_HPP
