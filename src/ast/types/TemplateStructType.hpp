#ifndef GULC_TEMPLATESTRUCTTYPE_HPP
#define GULC_TEMPLATESTRUCTTYPE_HPP

#include <ast/Type.hpp>
#include <ast/decls/TemplateStructDecl.hpp>

namespace gulc {
    /**
     * `TemplateStructType` is made to be used within uninstantiated template contexts.
     *
     * We use this type to allow us to resolve `TemplatedType` to its exact `Template*Decl` without having to
     * completely instantiate it. This makes it easier to validate templates while still making it easy to instantiate
     * templates (i.e. we will replace all `TemplateStructType` with `StructType` after instantiating the template it is
     * stored in).
     */
    class TemplateStructType : public Type {
    public:
        static bool classof(const Type* type) { return type->getTypeKind() == Type::Kind::TemplateStruct; }

        TemplateStructType(Qualifier qualifier, std::vector<Expr*> templateArguments, TemplateStructDecl* decl,
                           TextPosition startPosition, TextPosition endPosition)
                : Type(Type::Kind::TemplateStruct, qualifier, false),
                  _templateArguments(std::move(templateArguments)), _decl(decl),
                  _startPosition(startPosition), _endPosition(endPosition) {}

        std::vector<Expr*>& templateArguments() { return _templateArguments; }
        std::vector<Expr*> const& templateArguments() const { return _templateArguments; }
        TemplateStructDecl* decl() { return _decl; }
        TemplateStructDecl const* decl() const { return _decl; }
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

            auto result = new TemplateStructType(_qualifier, copiedTemplateArguments, _decl,
                                                 _startPosition, _endPosition);
            result->setIsLValue(_isLValue);
            return result;
        }

        ~TemplateStructType() override {
            for (Expr* templateArgument : _templateArguments) {
                delete templateArgument;
            }
        }

    protected:
        std::vector<Expr*> _templateArguments;
        TemplateStructDecl* _decl;
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_TEMPLATESTRUCTTYPE_HPP
