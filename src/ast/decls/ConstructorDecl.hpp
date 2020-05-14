#ifndef GULC_CONSTRUCTORDECL_HPP
#define GULC_CONSTRUCTORDECL_HPP

#include "FunctionDecl.hpp"

namespace gulc {
    enum ConstructorType {
        Normal,
        Copy,
        Move

    };

    class ConstructorDecl : public FunctionDecl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::Constructor; }

        ConstructorDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                        bool isConstExpr, Identifier identifier, DeclModifiers declModifiers,
                        std::vector<ParameterDecl*> parameters,
                        std::vector<Cont*> contracts, CompoundStmt* body,
                        TextPosition startPosition, TextPosition endPosition,
                        ConstructorType constructorType)
                : FunctionDecl(Decl::Kind::Constructor, sourceFileID, std::move(attributes), visibility,
                               isConstExpr, std::move(identifier), declModifiers, std::move(parameters), nullptr,
                               std::move(contracts), body, startPosition, endPosition),
                  _constructorType (constructorType) {}

        ConstructorType constructorType() const { return _constructorType; }

        Decl* deepCopy() const override {
            std::vector<Attr*> copiedAttributes;
            copiedAttributes.reserve(_attributes.size());
            std::vector<ParameterDecl*> copiedParameters;
            copiedParameters.reserve(_parameters.size());
            std::vector<Cont*> copiedContracts;
            copiedContracts.reserve(_contracts.size());

            for (Attr* attribute : _attributes) {
                copiedAttributes.push_back(attribute->deepCopy());
            }

            for (ParameterDecl* parameter : _parameters) {
                copiedParameters.push_back(llvm::dyn_cast<ParameterDecl>(parameter->deepCopy()));
            }

            for (Cont* contract : _contracts) {
                copiedContracts.push_back(contract->deepCopy());
            }

            auto result = new ConstructorDecl(_sourceFileID, copiedAttributes, _declVisibility, _isConstExpr,
                                              _identifier, _declModifiers, copiedParameters,
                                              copiedContracts, llvm::dyn_cast<CompoundStmt>(_body->deepCopy()),
                                              _startPosition, _endPosition, _constructorType);
            result->container = container;
            result->containedInTemplate = containedInTemplate;
            result->originalDecl = (originalDecl == nullptr ? this : originalDecl);
            return result;
        }

    protected:
        ConstructorType _constructorType;

    };
}

#endif //GULC_CONSTRUCTORDECL_HPP
