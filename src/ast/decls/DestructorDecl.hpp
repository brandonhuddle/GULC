#ifndef GULC_DESTRUCTORDECL_HPP
#define GULC_DESTRUCTORDECL_HPP

#include "FunctionDecl.hpp"

namespace gulc {
    class DestructorDecl : public FunctionDecl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::Destructor; }

        DestructorDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                       bool isConstExpr, Identifier identifier, DeclModifiers declModifiers,
                       std::vector<Cont*> contracts, CompoundStmt* body,
                       TextPosition startPosition, TextPosition endPosition)
                : FunctionDecl(Decl::Kind::Destructor, sourceFileID, std::move(attributes), visibility,
                               isConstExpr, std::move(identifier), declModifiers, {}, nullptr,
                               std::move(contracts), body, startPosition, endPosition) {}

    };
}

#endif //GULC_DESTRUCTORDECL_HPP
