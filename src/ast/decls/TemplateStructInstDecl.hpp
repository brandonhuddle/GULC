#ifndef GULC_TEMPLATESTRUCTINSTDECL_HPP
#define GULC_TEMPLATESTRUCTINSTDECL_HPP

#include "StructDecl.hpp"

namespace gulc {
    /**
     * A `TemplateStructInstDecl` is an instantiation of a `TemplateStructDecl`
     *
     * `Example<i32>` would be an instantiation of `struct Example<G>`
     */
    class TemplateStructInstDecl : public StructDecl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::TemplateStructInst; }

        std::vector<Expr*>& templateArguments() { return _templateArguments; }
        std::vector<Expr*> const& templateArguments() const { return _templateArguments; }

        TemplateStructInstDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                               bool isConstExpr, Identifier identifier,
                               TextPosition startPosition, TextPosition endPosition,
                               bool isClass, std::vector<Type*> inheritedTypes, std::vector<Cont*> contracts,
                               std::vector<Decl*> allMembers, std::vector<ConstructorDecl*> constructors,
                               DestructorDecl* destructor, std::vector<Expr*> templateArguments)
                : StructDecl(Decl::Kind::TemplateStructInst, sourceFileID, std::move(attributes), visibility,
                             isConstExpr, std::move(identifier), startPosition, endPosition, isClass,
                             std::move(inheritedTypes), std::move(contracts), std::move(allMembers),
                             std::move(constructors), destructor),
                  _templateArguments(std::move(templateArguments)) {}

        ~TemplateStructInstDecl() override {
            for (Expr* templateArgument : _templateArguments) {
                delete templateArgument;
            }
        }

    protected:
        std::vector<Expr*> _templateArguments;

    };
}

#endif //GULC_TEMPLATESTRUCTINSTDECL_HPP
