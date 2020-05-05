#ifndef GULC_ENUMCONSTDECL_HPP
#define GULC_ENUMCONSTDECL_HPP

#include <ast/Decl.hpp>
#include <ast/Expr.hpp>

namespace gulc {
    class EnumConstDecl : public Decl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::EnumConst; }

        // NOTE: This can be null at some points of execution...
        Expr* constValue;

        EnumConstDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Identifier identifier,
                      TextPosition startPosition, TextPosition endPosition, Expr* constValue)
                : Decl(Kind::EnumConst, sourceFileID, std::move(attributes),
                       Decl::Visibility::Unassigned, false, std::move(identifier),
                       DeclModifiers::None),
                  _startPosition(startPosition), _endPosition(endPosition), constValue(constValue) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Decl* deepCopy() const override {
            std::vector<Attr*> copiedAttributes;
            copiedAttributes.reserve(_attributes.size());
            Expr* copiedConstValue = nullptr;

            for (Attr* attribute : _attributes) {
                copiedAttributes.push_back(attribute->deepCopy());
            }

            if (constValue != nullptr) {
                copiedConstValue = constValue->deepCopy();
            }

            auto result = new EnumConstDecl(_sourceFileID, copiedAttributes, _identifier,
                                            _startPosition, _endPosition, copiedConstValue);
            result->container = container;
            result->containedInTemplate = containedInTemplate;
            return result;
        }

        ~EnumConstDecl() override {
            delete constValue;
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_ENUMCONSTDECL_HPP
