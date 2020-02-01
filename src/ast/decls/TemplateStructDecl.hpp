#ifndef GULC_TEMPLATESTRUCTDECL_HPP
#define GULC_TEMPLATESTRUCTDECL_HPP

#include <vector>
#include "StructDecl.hpp"
#include "TemplateParameterDecl.hpp"

namespace gulc {
    class TemplateStructDecl : public StructDecl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::TemplateStruct; }

        std::vector<TemplateParameterDecl*>& templateParameters() { return _templateParameters; }
        std::vector<TemplateParameterDecl*> const& templateParameters() const { return _templateParameters; }

        TemplateStructDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                           bool isConstExpr, Identifier identifier,
                           TextPosition startPosition, TextPosition endPosition,
                           bool isClass, std::vector<Type*> inheritedTypes, std::vector<Cont*> contracts,
                           std::vector<Decl*> allMembers, std::vector<ConstructorDecl*> constructors,
                           DestructorDecl* destructor, std::vector<TemplateParameterDecl*> templateParameters)
                : StructDecl(Decl::Kind::TemplateStruct, sourceFileID, std::move(attributes), visibility,
                             isConstExpr, std::move(identifier), startPosition, endPosition, isClass,
                             std::move(inheritedTypes), std::move(contracts), std::move(allMembers),
                             std::move(constructors), destructor),
                  _templateParameters(std::move(templateParameters)) {}

        ~TemplateStructDecl() override {
            for (TemplateParameterDecl* templateParameter : _templateParameters) {
                delete templateParameter;
            }
        }

    protected:
        std::vector<TemplateParameterDecl*> _templateParameters;

    };
}

#endif //GULC_TEMPLATESTRUCTDECL_HPP
