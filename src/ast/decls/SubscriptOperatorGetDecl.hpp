#ifndef GULC_SUBSCRIPTOPERATORGETDECL_HPP
#define GULC_SUBSCRIPTOPERATORGETDECL_HPP

#include "FunctionDecl.hpp"

namespace gulc {
    class SubscriptOperatorGetDecl : public FunctionDecl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::SubscriptOperatorGet; }

        enum class GetResult {
            Normal, // `get`
            Ref, // `get ref`
            RefMut // `get ref mut`
        };

        SubscriptOperatorGetDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                                 bool isConstExpr, Identifier identifier, DeclModifiers declModifiers,
                                 Type* returnType, std::vector<Cont*> contracts, CompoundStmt* body,
                                 TextPosition startPosition, TextPosition endPosition, GetResult getResult)
                : FunctionDecl(Decl::Kind::SubscriptOperatorGet, sourceFileID, std::move(attributes),
                               visibility, isConstExpr, std::move(identifier), declModifiers, {}, returnType,
                               std::move(contracts), body, startPosition, endPosition),
                  _getResult(getResult) {}

        GetResult getResultType() const { return _getResult; }

        Decl* deepCopy() const override {
            std::vector<Attr*> copiedAttributes;
            copiedAttributes.reserve(_attributes.size());
            std::vector<Cont*> copiedContracts;
            copiedContracts.reserve(_contracts.size());

            for (Attr* attribute : _attributes) {
                copiedAttributes.push_back(attribute->deepCopy());
            }

            for (Cont* contract : _contracts) {
                copiedContracts.push_back(contract->deepCopy());
            }

            auto result = new SubscriptOperatorGetDecl(_sourceFileID, copiedAttributes, _declVisibility, _isConstExpr,
                                                       _identifier, _declModifiers,
                                                       returnType->deepCopy(), copiedContracts,
                                                       llvm::dyn_cast<CompoundStmt>(_body->deepCopy()),
                                                       _startPosition, _endPosition, _getResult);
            result->container = container;
            result->containedInTemplate = containedInTemplate;
            return result;
        }

    protected:
        GetResult _getResult;

    };
}

#endif //GULC_SUBSCRIPTOPERATORGETDECL_HPP
