#ifndef GULC_MANGLERBASE_HPP
#define GULC_MANGLERBASE_HPP

#include <ast/decls/EnumDecl.hpp>
#include <ast/decls/StructDecl.hpp>
#include <ast/decls/NamespaceDecl.hpp>
#include <ast/decls/TemplateFunctionDecl.hpp>
#include <ast/decls/OperatorDecl.hpp>
#include <ast/decls/CallOperatorDecl.hpp>
#include <ast/decls/SubscriptOperatorDecl.hpp>
#include <ast/decls/TraitDecl.hpp>

namespace gulc {
    // I'm making the name mangler an abstract class to allow us to use Itanium on nearly every platform but on Windows
    // On windows I would at least like the option to try and match the Windows C++ ABI. From what I can tell the
    // Windows C++ ABI doesn't have public documentation except various documentation made from people reverse
    // engineering it throughout the years. I might have to drive to Redwood and try to become friends with a Microsoft
    // employee at some point...
    class ManglerBase {
    public:
        // We have to do a prepass on declared types to mangle their names because we will need to access them as
        // parameters in the function signature
        virtual void mangleDecl(EnumDecl* enumDecl) = 0;
        virtual void mangleDecl(StructDecl* structDecl) = 0;
        virtual void mangleDecl(TraitDecl* traitDecl) = 0;
        virtual void mangleDecl(NamespaceDecl* namespaceDecl) = 0;

        virtual void mangle(FunctionDecl* functionDecl) = 0;
        virtual void mangle(VariableDecl* variableDecl) = 0;
        virtual void mangle(NamespaceDecl* namespaceDecl) = 0;
        virtual void mangle(StructDecl* structDecl) = 0;
        virtual void mangle(TraitDecl* traitDecl) = 0;
        virtual void mangle(CallOperatorDecl* callOperatorDecl) = 0;

        virtual ~ManglerBase() = default;

    };
}

#endif //GULC_MANGLERBASE_HPP
