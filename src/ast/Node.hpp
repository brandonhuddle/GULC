#ifndef GULC_NODE_HPP
#define GULC_NODE_HPP

namespace gulc {
    struct TextPosition {
        unsigned int index;
        unsigned int line;
        unsigned int column;

        TextPosition()
                : index(0), line(0), column(0) {}

        TextPosition(unsigned int index, unsigned int line, unsigned int column)
                : index(index), line(line), column(column) {}
    };

    /**
     * Node is the base AST class for all AST objects. It contains all properties required by all other nodes
     */
    class Node {
    public:
        enum class Kind {
            Attr,
            Cont,
            Decl,
            Expr,
            Identifier,
            Stmt,
            Type
        };

        Kind getNodeKind() const { return _nodeKind; }

        virtual TextPosition startPosition() const = 0;
        virtual TextPosition endPosition() const = 0;

        virtual ~Node() = default;

    protected:
        Kind _nodeKind;

        explicit Node(Kind kind) : _nodeKind(kind) {}

    };
}

#endif //GULC_NODE_HPP
