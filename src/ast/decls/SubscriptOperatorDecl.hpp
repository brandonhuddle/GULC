#ifndef GULC_SUBSCRIPTOPERATORDECL_HPP
#define GULC_SUBSCRIPTOPERATORDECL_HPP

#include <ast/Decl.hpp>
#include <ast/Type.hpp>
#include <ast/DeclModifiers.hpp>
#include <llvm/Support/Casting.h>
#include "SubscriptOperatorGetDecl.hpp"
#include "SubscriptOperatorSetDecl.hpp"

namespace gulc {
    class SubscriptOperatorDecl : public Decl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::SubscriptOperator; }

        // The type that will be used for the generic `get`/`set`
        Type* type;

        SubscriptOperatorDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                              bool isConstExpr, Identifier identifier, std::vector<ParameterDecl*> parameters, Type* type,
                              TextPosition startPosition, TextPosition endPosition, DeclModifiers declModifiers,
                              std::vector<SubscriptOperatorGetDecl*> getters, SubscriptOperatorSetDecl* setter)
                : Decl(Decl::Kind::SubscriptOperator, sourceFileID, std::move(attributes), visibility,
                       isConstExpr, std::move(identifier), declModifiers),
                  type(type), _parameters(std::move(parameters)), _startPosition(startPosition),
                  _endPosition(endPosition), _getters(std::move(getters)), _setter(setter) {
            for (SubscriptOperatorGetDecl* getter : _getters) {
                getter->container = this;
            }

            if (_setter != nullptr) {
                _setter->container = this;
            }
        }

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        std::vector<ParameterDecl*>& parameters() { return _parameters; }
        std::vector<ParameterDecl*> const& parameters() const { return _parameters; }

        std::vector<SubscriptOperatorGetDecl*>& getters() { return _getters; }
        std::vector<SubscriptOperatorGetDecl*> const& getters() const { return _getters; }
        SubscriptOperatorSetDecl* setter() { return _setter; }
        bool hasSetter() const { return _setter != nullptr; }

        Decl* deepCopy() const override {
            std::vector<Attr*> copiedAttributes;
            copiedAttributes.reserve(_attributes.size());
            std::vector<ParameterDecl*> copiedParameters;
            copiedParameters.reserve(_parameters.size());
            std::vector<SubscriptOperatorGetDecl*> copiedGetters;
            copiedGetters.reserve(_getters.size());
            SubscriptOperatorSetDecl* copiedSetter = nullptr;

            for (Attr* attribute : _attributes) {
                copiedAttributes.push_back(attribute->deepCopy());
            }

            for (ParameterDecl* parameter : _parameters) {
                copiedParameters.push_back(llvm::dyn_cast<ParameterDecl>(parameter->deepCopy()));
            }

            for (SubscriptOperatorGetDecl* getter : _getters) {
                copiedGetters.push_back(llvm::dyn_cast<SubscriptOperatorGetDecl>(getter->deepCopy()));
            }

            if (_setter != nullptr) {
                copiedSetter = llvm::dyn_cast<SubscriptOperatorSetDecl>(_setter->deepCopy());
            }

            auto result = new SubscriptOperatorDecl(_sourceFileID, copiedAttributes, _declVisibility, _isConstExpr,
                                                    _identifier, copiedParameters,
                                                    type->deepCopy(), _startPosition, _endPosition,
                                                    _declModifiers, copiedGetters, copiedSetter);
            result->container = container;
            result->containedInTemplate = containedInTemplate;
            return result;
        }

        ~SubscriptOperatorDecl() override {
            for (ParameterDecl* parameter : _parameters) {
                delete parameter;
            }

            delete type;
            delete _setter;

            for (SubscriptOperatorGetDecl* getter : _getters) {
                delete getter;
            }
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;
        std::vector<ParameterDecl*> _parameters;
        std::vector<SubscriptOperatorGetDecl*> _getters;
        SubscriptOperatorSetDecl* _setter;

    };
}

#endif //GULC_SUBSCRIPTOPERATORDECL_HPP
