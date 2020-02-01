#ifndef GULC_NAMESPACEDECL_HPP
#define GULC_NAMESPACEDECL_HPP

#include <ast/Decl.hpp>
#include <llvm/Support/Casting.h>

namespace gulc {
    class NamespaceDecl : public Decl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::Namespace; }

        NamespaceDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Identifier identifier,
                      TextPosition startPosition, TextPosition endPosition)
                : Decl(Kind::Namespace, sourceFileID, std::move(attributes),
                       Decl::Visibility::Unassigned, false, std::move(identifier)),
                  _startPosition(startPosition), _endPosition(endPosition), _nestedDecls(), _isPrototype(false) {}

        std::vector<Decl*>& nestedDecls() { return _nestedDecls; }
        const std::vector<Decl*>& nestedDecls() const { return _nestedDecls; }

        void addNestedDecl(Decl* nestedDecl) { _nestedDecls.push_back(nestedDecl); }

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        ~NamespaceDecl() override {
            for (Decl* decl : _nestedDecls) {
                if (_isPrototype && !llvm::isa<NamespaceDecl>(decl)) {
                    // We're not allowed to delete anything but namespaces if the namespace is a prototype
                    continue;
                }

                delete decl;
            }
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;
        std::vector<Decl*> _nestedDecls;
        // If this is true it means we only own a nested `Decl` if it is a namespace, all other cannot be deleted by us.
        bool _isPrototype;

    };
}

#endif //GULC_NAMESPACEDECL_HPP
