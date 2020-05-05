#ifndef GULC_SUBSCRIPTOPERATORSETDECL_HPP
#define GULC_SUBSCRIPTOPERATORSETDECL_HPP

#include "FunctionDecl.hpp"

namespace gulc {
    class SubscriptOperatorSetDecl : public FunctionDecl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::SubscriptOperatorSet; }

        SubscriptOperatorSetDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                                 bool isConstExpr, Identifier identifier, DeclModifiers declModifiers,
                                 Type* paramType, std::vector<Cont*> contracts, CompoundStmt* body,
                                 TextPosition startPosition, TextPosition endPosition)
                : FunctionDecl(Decl::Kind::SubscriptOperatorSet, sourceFileID, std::move(attributes), visibility,
                               isConstExpr, std::move(identifier), declModifiers,
                // Create the only parameter needed, `_ value: $returnType`
                               { new ParameterDecl(sourceFileID, {}, Identifier({}, {}, "_"),
                                                   Identifier({}, {}, "value"), paramType,
                                                   nullptr, ParameterDecl::ParameterKind::Val,
                                                   {}, {}) },
                               nullptr, std::move(contracts), body, startPosition, endPosition) {}

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

            auto result = new SubscriptOperatorSetDecl(_sourceFileID, copiedAttributes, _declVisibility, _isConstExpr,
                                                       _identifier, _declModifiers,
                                                       returnType->deepCopy(), copiedContracts,
                                                       llvm::dyn_cast<CompoundStmt>(_body->deepCopy()),
                                                       _startPosition, _endPosition);
            result->container = container;
            result->containedInTemplate = containedInTemplate;
            return result;
        }

    };
}

#endif //GULC_SUBSCRIPTOPERATORSETDECL_HPP
