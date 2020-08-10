#ifndef GULC_ITANIUMMANGLER_HPP
#define GULC_ITANIUMMANGLER_HPP

#include "ManglerBase.hpp"

namespace gulc {
    // I try my best to keep the Ghoul ABI as close to the C++ Itanium ABI spec but there are bound to be areas where
    // the languages differ too much. In situations where the languages can't be a perfect match I will try my best to
    // at least come close to matching the Itanium spec.
    class ItaniumMangler : public ManglerBase {
    public:
        void mangleDecl(EnumDecl* enumDecl) override;
        void mangleDecl(StructDecl* structDecl) override;
        void mangleDecl(TraitDecl* traitDecl) override;
        void mangleDecl(NamespaceDecl* namespaceDecl) override;

        void mangle(FunctionDecl* functionDecl) override;
        void mangle(VariableDecl* variableDecl) override;
        void mangle(NamespaceDecl* namespaceDecl) override;
        void mangle(StructDecl* structDecl) override;
        void mangle(TraitDecl* traitDecl) override;
        void mangle(CallOperatorDecl* callOperatorDecl) override;

    private:
        void mangleDeclEnum(EnumDecl* enumDecl, std::string const& prefix, std::string const& nameSuffix);
        void mangleDeclStruct(StructDecl* structDecl, std::string const& prefix, std::string const& nameSuffix);
        void mangleDeclTrait(TraitDecl* traitDecl, std::string const& prefix, std::string const& nameSuffix);
        void mangleDeclNamespace(NamespaceDecl* namespaceDecl, std::string const& prefix);

        void mangleFunction(FunctionDecl* functionDecl, std::string const& prefix, std::string const& nameSuffix);
        void mangleVariable(VariableDecl* variableDecl, std::string const& prefix, std::string const& nameSuffix);
        void mangleNamespace(NamespaceDecl* namespaceDecl, std::string const& prefix);
        void mangleStruct(StructDecl* structDecl, std::string const& prefix);
        void mangleTrait(TraitDecl* traitDecl, std::string const& prefix);
        void mangleCallOperator(CallOperatorDecl* callOperatorDecl, std::string const& prefix,
                                std::string const& nameSuffix);

        void mangleConstructor(ConstructorDecl* constructorDecl, std::string const& prefix,
                               std::string const& nameSuffix);
        void mangleDestructor(DestructorDecl* destructorDecl, std::string const& prefix, std::string const& nameSuffix);

        std::string unqualifiedName(FunctionDecl* functionDecl);
        std::string unqualifiedName(VariableDecl* variableDecl);

        std::string sourceName(std::string const& s);
        std::string bareFunctionType(std::vector<ParameterDecl*>& params);
        std::string typeName(gulc::Type* type);

        std::string templateArgs(std::vector<TemplateParameterDecl*>& templateParams, std::vector<Expr*>& templateArgs);
        std::string templateArg(Expr const* expr);
        std::string exprPrimary(Expr const* expr);

        std::string operatorName(OperatorType operatorType, std::string const& operatorText);

    };
}

#endif //GULC_ITANIUMMANGLER_HPP
