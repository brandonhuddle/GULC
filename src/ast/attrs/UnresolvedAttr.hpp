#ifndef GULC_UNRESOLVEDATTR_HPP
#define GULC_UNRESOLVEDATTR_HPP

#include <ast/Attr.hpp>
#include <ast/Identifier.hpp>
#include <vector>
#include <ast/Expr.hpp>

namespace gulc {
    class UnresolvedAttr : public Attr {
    public:
        static bool classof(const Attr* attr) { return attr->getAttrKind() == Attr::Kind::Unresolved; }

        std::vector<Expr*> arguments;

        UnresolvedAttr(TextPosition startPosition, TextPosition endPosition,
                       std::vector<Identifier> namespacePath, Identifier identifier, std::vector<Expr*> arguments)
                : Attr(Attr::Kind::Unresolved, startPosition, endPosition),
                  arguments(std::move(arguments)), _namespacePath(std::move(namespacePath)),
                  _identifier(std::move(identifier)) {}

        std::vector<Identifier> const& namespacePath() const { return _namespacePath; }
        Identifier const& identifier() const { return _identifier; }

        Attr* deepCopy() const override {
            std::vector<Expr*> copiedArguments;
            copiedArguments.reserve(arguments.size());

            for (Expr* argument : arguments) {
                copiedArguments.push_back(argument->deepCopy());
            }

            return new UnresolvedAttr(_startPosition, _endPosition, _namespacePath,
                                      _identifier, copiedArguments);
        }

        ~UnresolvedAttr() override {
            for (Expr* argument : arguments) {
                delete argument;
            }
        }

    protected:
        std::vector<Identifier> _namespacePath;
        Identifier _identifier;

    };
}

#endif //GULC_UNRESOLVEDATTR_HPP
