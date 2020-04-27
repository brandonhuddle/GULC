#ifndef GULC_TYPEALIASDECL_HPP
#define GULC_TYPEALIASDECL_HPP

#include <ast/Decl.hpp>
#include <ast/Type.hpp>
#include <llvm/Support/Casting.h>
#include <ast/Expr.hpp>
#include <ast/exprs/TypeExpr.hpp>
#include <iostream>
#include <utilities/TemplateInstHelper.hpp>
#include "TemplateParameterDecl.hpp"

namespace gulc {
    // We will probably only ever support `prefix` but `infix` may be useful at some point. It could be used for
    // implementing syntax similar to `i32 | f32` among other things.
    enum class TypeAliasType {
        Normal,
        Prefix
    };

    class TypeAliasDecl : public Decl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::TypeAlias; }

        // This is the `= x;` part of the declaration.
        Type* typeValue;
        TypeAliasType typeAliasType() const { return _typeAliasType; }
        std::vector<TemplateParameterDecl*>& templateParameters() { return _templateParameters; }
        std::vector<TemplateParameterDecl*> const& templateParameters() const { return _templateParameters; }
        bool hasTemplateParameters() const { return !templateParameters().empty(); }

        TypeAliasDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                      TypeAliasType typeAliasType, Identifier identifier,
                      std::vector<TemplateParameterDecl*> templateParameters, Type* typeValue,
                      TextPosition startPosition, TextPosition endPosition)
                : Decl(Decl::Kind::TypeAlias, sourceFileID, std::move(attributes), visibility, true,
                       std::move(identifier)),
                  typeValue(typeValue), _startPosition(startPosition), _endPosition(endPosition),
                  _typeAliasType(typeAliasType), _templateParameters(std::move(templateParameters)) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Type* getInstantiation(std::vector<Expr*> const& templateArguments) {
            // While checking the arguments are valid we will also create a copy of the arguments
            std::vector<Expr*> copiedTemplateArguments;
            copiedTemplateArguments.reserve(templateArguments.size());

            for (std::size_t i = 0; i < _templateParameters.size(); ++i) {
                if (_templateParameters[i]->templateParameterKind() == TemplateParameterDecl::TemplateParameterKind::Typename) {
                    if (!llvm::isa<TypeExpr>(templateArguments[i])) {
                        std::cerr << "INTERNAL ERROR: `TemplateTraitDecl::getInstantiation` received non-type argument where type was expected!" << std::endl;
                        std::exit(1);
                    }
                } else {
                    // Const
                    if (llvm::isa<TypeExpr>(templateArguments[i])) {
                        std::cerr << "INTERNAL ERROR: `TemplateTraitDecl::getInstantiation` received type argument where const literal was expected!" << std::endl;
                        std::exit(1);
                    }
                }

                copiedTemplateArguments.push_back(templateArguments[i]->deepCopy());
            }

            Type* result = typeValue->deepCopy();

            TemplateInstHelper templateInstHelper;
            templateInstHelper.instantiateType(result, &_templateParameters, &copiedTemplateArguments);

            // TODO: `result` doesn't contain a `TemplatedType` somewhere then `copiedTemplateArguments` will be leaky

            return result;
        }

        Decl* deepCopy() const override {
            std::vector<Attr*> copiedAttributes;
            copiedAttributes.reserve(_attributes.size());
            std::vector<TemplateParameterDecl*> copiedTemplateParameters;
            copiedTemplateParameters.reserve(_templateParameters.size());

            for (Attr* attribute : _attributes) {
                copiedAttributes.push_back(attribute->deepCopy());
            }

            for (TemplateParameterDecl* templateParameter : _templateParameters) {
                copiedTemplateParameters.push_back(llvm::dyn_cast<TemplateParameterDecl>(templateParameter->deepCopy()));
            }

            return new TypeAliasDecl(_sourceFileID, copiedAttributes, _declVisibility, _typeAliasType,
                                     _identifier, copiedTemplateParameters, typeValue->deepCopy(),
                                    _startPosition, _endPosition);
        }

        ~TypeAliasDecl() override {
            delete typeValue;

            for (TemplateParameterDecl* templateParameter : _templateParameters) {
                delete templateParameter;
            }
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;
        TypeAliasType _typeAliasType;
        std::vector<TemplateParameterDecl*> _templateParameters;

    };
}

#endif //GULC_TYPEALIASDECL_HPP
