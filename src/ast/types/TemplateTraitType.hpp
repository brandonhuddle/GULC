#ifndef GULC_TEMPLATETRAITTYPE_HPP
#define GULC_TEMPLATETRAITTYPE_HPP

#include <ast/Type.hpp>
#include <ast/decls/TemplateTraitDecl.hpp>

namespace gulc {
    /**
     * `TemplateTraitType` is made to be used within uninstantiated template contexts.
     *
     * We use this type to allow us to resolve `TemplatedType` to its exact `Template*Decl` without having to
     * completely instantiate it. This makes it easier to validate templates while still making it easy to instantiate
     * templates (i.e. we will replace all `TemplateTraitType` with `TraitType` after instantiating the template it is
     * stored in).
     */
    class TemplateTraitType : public Type {
    public:
        static bool classof(const Type* type) { return type->getTypeKind() == Type::Kind::TemplateTrait; }

        TemplateTraitType(Qualifier qualifier, std::vector<Expr*> templateArguments, TemplateTraitDecl* decl,
                          TextPosition startPosition, TextPosition endPosition)
                : Type(Type::Kind::TemplateTrait, qualifier, false),
                  _templateArguments(std::move(templateArguments)), _decl(decl),
                  _startPosition(startPosition), _endPosition(endPosition) {}

        std::vector<Expr*>& templateArguments() { return _templateArguments; }
        std::vector<Expr*> const& templateArguments() const { return _templateArguments; }
        TemplateTraitDecl* decl() { return _decl; }
        TemplateTraitDecl const* decl() const { return _decl; }
        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        std::string toString() const override {
            return _decl->identifier().name();
        }

        Type* deepCopy() const override {
            std::vector<Expr*> copiedTemplateArguments;

            for (Expr* templateArgument : _templateArguments) {
                copiedTemplateArguments.push_back(templateArgument->deepCopy());
            }

            auto result = new TemplateTraitType(_qualifier, copiedTemplateArguments, _decl,
                                                _startPosition, _endPosition);
            result->setIsLValue(_isLValue);
            return result;
        }

        ~TemplateTraitType() override {
            for (Expr* templateArgument : _templateArguments) {
                delete templateArgument;
            }
        }

    protected:
        std::vector<Expr*> _templateArguments;
        TemplateTraitDecl* _decl;
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_TEMPLATETRAITTYPE_HPP
