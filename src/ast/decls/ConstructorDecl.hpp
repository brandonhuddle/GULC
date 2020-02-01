#ifndef GULC_CONSTRUCTORDECL_HPP
#define GULC_CONSTRUCTORDECL_HPP

#include "FunctionDecl.hpp"

namespace gulc {
    class ConstructorDecl : public FunctionDecl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::Constructor; }

        ConstructorDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                        bool isConstExpr, Identifier identifier, DeclModifiers declModifiers,
                        std::vector<ParameterDecl*> parameters,
                        std::vector<Cont*> contracts, CompoundStmt* body,
                        TextPosition startPosition, TextPosition endPosition)
                : FunctionDecl(Decl::Kind::Constructor, sourceFileID, std::move(attributes), visibility,
                               isConstExpr, std::move(identifier), declModifiers, std::move(parameters), nullptr,
                               std::move(contracts), body, startPosition, endPosition) {}

    };
}

#endif //GULC_CONSTRUCTORDECL_HPP
