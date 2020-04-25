#ifndef GULC_DECL_HPP
#define GULC_DECL_HPP

#include <string>
#include <vector>
#include <cassert>
#include "Node.hpp"
#include "Identifier.hpp"
#include "Attr.hpp"

namespace gulc {
    class Decl : public Node {
    public:
        static bool classof(const Node* node) { return node->getNodeKind() == Node::Kind::Decl; }

        enum class Kind {
            Import,

            Function,
            TemplateFunction,

            Property,
            PropertyGet,
            PropertySet,

            Operator,
            CastOperator,
            CallOperator,
            SubscriptOperator,
            SubscriptOperatorGet,
            SubscriptOperatorSet,

            Constructor,
            Destructor,

            Struct,
            TemplateStruct,
            TemplateStructInst,
            Trait,
            TemplateTrait,
            Union,

            Attribute,

            Namespace,

            Enum,
            EnumConstant,

            Variable,

            Parameter,
            TemplateParameter
        };

        enum class Visibility {
            Unassigned,
            Public,
            Private,
            Protected,
            Internal,
            ProtectedInternal
        };

        Kind getDeclKind() const { return _declKind; }
        unsigned int sourceFileID() const { return _sourceFileID; }
        std::vector<Attr*>& attributes() { return _attributes; }
        std::vector<Attr*> const& attributes() const { return _attributes; }
        Visibility visibility() const { return _declVisibility; }
        Identifier const& identifier() const { return _identifier; }
        // `const` in GUL, `constexpr` in ulang
        bool isConstExpr() const { return _isConstExpr; }

        virtual Decl* deepCopy() const = 0;

        void setAttributes(std::vector<Attr*> attributes) {
            assert(_attributes.empty());

            _attributes = std::move(attributes);
        }

    protected:
        Kind _declKind;
        unsigned int _sourceFileID;
        std::vector<Attr*> _attributes;
        Visibility _declVisibility;
        Identifier _identifier;
        bool _isConstExpr;

        Decl(Kind declKind, unsigned int sourceFileID, std::vector<Attr*> attributes, Visibility declVisibility,
             bool isConstExpr, Identifier identifier)
                : Decl(Node::Kind::Decl, declKind, sourceFileID, std::move(attributes), declVisibility,
                       isConstExpr, std::move(identifier)) {}
        Decl(Node::Kind nodeKind, Kind declKind, unsigned int sourceFileID, std::vector<Attr*> attributes,
             Visibility declVisibility, bool isConstExpr, Identifier identifier)
                : Node(nodeKind), _declKind(declKind), _declVisibility(declVisibility), _sourceFileID(sourceFileID),
                  _attributes(std::move(attributes)), _isConstExpr(isConstExpr), _identifier(std::move(identifier)) {}

    };
}

#endif //GULC_DECL_HPP
