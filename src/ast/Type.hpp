#ifndef GULC_TYPE_HPP
#define GULC_TYPE_HPP

#include <string>
#include "Node.hpp"

namespace gulc {
    class Type : public Node {
    public:
        static bool classof(const Node* node) { return node->getNodeKind() == Node::Kind::Type; }

        enum class Kind {
            Alias,
            BuiltIn,
            Dependent,
            Dimension,
            Enum,
            FlatArray,
            Pointer,
            Reference,
            Struct,
            Templated,
            TemplateStruct,
            TemplateTrait,
            TemplateTypenameRef,
            Trait,
            Unresolved,
            UnresolvedNested,
            VTable
        };

        enum class Qualifier {
            Unassigned,
            Mut,
            Immut
        };

        Kind getTypeKind() const { return _typeKind; }
        Qualifier qualifier() const { return _qualifier; }
        /// NOTE: Function return values are always stored into temporary values, making them an `lvalue` but they are
        ///       ALWAYS `const`, making them an unassignable `lvalue`.
        bool isLValue() const { return _isLValue; }
        void setQualifier(Qualifier qualifier) { _qualifier = qualifier; }
        void setIsLValue(bool isLValue) { _isLValue = true; }

        virtual std::string toString() const = 0;
        virtual Type* deepCopy() const = 0;

    protected:
        Kind _typeKind;
        Qualifier _qualifier;
        bool _isLValue;

        Type(Type::Kind typeKind, Qualifier qualifier, bool isLValue)
                : Type(Node::Kind::Type, typeKind, qualifier, isLValue) {}
        Type(Node::Kind nodeKind, Type::Kind typeKind, Qualifier qualifier, bool isLValue)
                : Node(nodeKind), _typeKind(typeKind), _qualifier(qualifier), _isLValue(isLValue) {}

    };
}

#endif //GULC_TYPE_HPP
