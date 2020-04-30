#ifndef GULC_VARIABLEDECL_HPP
#define GULC_VARIABLEDECL_HPP

#include <ast/Decl.hpp>
#include <ast/Type.hpp>
#include <ast/Expr.hpp>
#include <ast/DeclModifiers.hpp>

namespace gulc {
    class VariableDecl : public Decl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::Variable; }

        Type* type;
        Expr* initialValue;

        bool hasInitialValue() const { return initialValue != nullptr; }
        DeclModifiers declModifiers() const { return _declModifiers; }

        VariableDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                     bool isConstExpr, Identifier identifier, DeclModifiers declModifiers,
                     Type* type, Expr* initialValue, TextPosition startPosition, TextPosition endPosition)
                : Decl(Decl::Kind::Variable, sourceFileID, std::move(attributes), visibility, isConstExpr,
                       std::move(identifier), declModifiers),
                  type(type), initialValue(initialValue), _startPosition(startPosition), _endPosition(endPosition) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Decl* deepCopy() const override {
            std::vector<Attr*> copiedAttributes;
            copiedAttributes.reserve(_attributes.size());
            Expr* copiedInitialValue = nullptr;

            for (Attr* attribute : _attributes) {
                copiedAttributes.push_back(attribute->deepCopy());
            }

            if (initialValue != nullptr) {
                copiedInitialValue = initialValue->deepCopy();
            }

            return new VariableDecl(_sourceFileID, copiedAttributes, _declVisibility, _isConstExpr, _identifier,
                                    _declModifiers, type->deepCopy(), copiedInitialValue,
                                    _startPosition, _endPosition);
        }

        ~VariableDecl() override {
            delete type;
            delete initialValue;
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_VARIABLEDECL_HPP
