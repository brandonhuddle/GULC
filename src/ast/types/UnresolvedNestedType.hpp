#ifndef GULC_UNRESOLVEDNESTEDTYPE_HPP
#define GULC_UNRESOLVEDNESTEDTYPE_HPP

#include <ast/Type.hpp>
#include <ast/Expr.hpp>
#include <vector>
#include <ast/Identifier.hpp>

namespace gulc {
    /**
     * Type to make unresolved types nested within other types or namespaces easier to represent
     */
    class UnresolvedNestedType : public Type {
    public:
        static bool classof(const Type* type) { return type->getTypeKind() == Type::Kind::UnresolvedNested; }

        // The container the type is nested within
        Type* container;

        UnresolvedNestedType(Qualifier qualifier, Type* container, Identifier nestedIdentifier,
                             std::vector<Expr*> templateArguments, TextPosition startPosition, TextPosition endPosition)
                : Type(Type::Kind::UnresolvedNested, qualifier, false),
                  container(container), _nestedIdentifier(std::move(nestedIdentifier)),
                  _templateArguments(std::move(templateArguments)),
                  _startPosition(startPosition), _endPosition(endPosition) {}

        Identifier const& identifier() const { return _nestedIdentifier; }
        std::vector<Expr*>& templateArguments() { return _templateArguments; }
        std::vector<Expr*> const& templateArguments() const { return _templateArguments; }
        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        std::string toString() const override {
            std::string templateArgsString;

            if (!_templateArguments.empty()) {
                templateArgsString = "<";

                for (std::size_t i = 0; i < _templateArguments.size(); ++i) {
                    if (i != 0) templateArgsString += ", ";

                    templateArgsString += _templateArguments[i]->toString();
                }

                templateArgsString += ">";
            }

            return container->toString() + "." + _nestedIdentifier.name() + templateArgsString;
        }

        Type* deepCopy() const override {
            std::vector<Expr*> copiedTemplateArguments;

            for (Expr* templateArgument : _templateArguments) {
                copiedTemplateArguments.push_back(templateArgument->deepCopy());
            }

            return new UnresolvedNestedType(_qualifier, container->deepCopy(), _nestedIdentifier,
                                            copiedTemplateArguments, _startPosition, _endPosition);
        }

        ~UnresolvedNestedType() override {
            delete container;

            for (Expr* templateArgument : _templateArguments) {
                delete templateArgument;
            }
        }

    protected:
        Identifier _nestedIdentifier;
        std::vector<Expr*> _templateArguments;
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_UNRESOLVEDNESTEDTYPE_HPP
