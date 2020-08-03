#ifndef GULC_TEMPLATEDTYPE_HPP
#define GULC_TEMPLATEDTYPE_HPP

#include <ast/Type.hpp>
#include <ast/Decl.hpp>
#include <ast/Expr.hpp>

namespace gulc {
    /**
     * This is a type that tries to allow us to support the complex template typing supported in GUL
     * Due to GUL supporting so many layers of compile-time solving and having template type parameters be qualified
     * using compounded traits, we're unable to fully solve
     *
     * To fully support the type system we have to have this type. It holds a collection of potential structs, traits,
     * etc. and the template parameters. Then later on we try to resolve this type again (once the parameters have been
     * completely resolved and it is possible to know what the solution is)
     */
    class TemplatedType : public Type {
    public:
        static bool classof(const Type* type) { return type->getTypeKind() == Type::Kind::Templated; }

        TemplatedType(Qualifier qualifier, std::vector<Decl*> matchingTemplateDecls,
                      std::vector<Expr*> templateArguments, TextPosition startPosition, TextPosition endPosition)
                : Type(Type::Kind::Templated, qualifier, false),
                  _matchingTemplateDecls(std::move(matchingTemplateDecls)),
                  _templateArguments(std::move(templateArguments)),
                  _startPosition(startPosition), _endPosition(endPosition) {}

        std::vector<Decl*>& matchingTemplateDecls() { return _matchingTemplateDecls; }
        std::vector<Decl*> const& matchingTemplateDecls() const { return _matchingTemplateDecls; }
        std::vector<Expr*>& templateArguments() { return _templateArguments; }
        std::vector<Expr*> const& templateArguments() const { return _templateArguments; }

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        std::string toString() const override {
            std::string templateArgumentsString = "<";

            for (std::size_t i = 0; i < _templateArguments.size(); ++i) {
                if (i != 0) templateArgumentsString += ", ";

                templateArgumentsString += _templateArguments[i]->toString();
            }

            templateArgumentsString += ">";

            return _matchingTemplateDecls[0]->identifier().name() + templateArgumentsString;
        }

        Type* deepCopy() const override {
            std::vector<Expr*> copiedTemplateArguments;

            for (Expr* templateArgument : _templateArguments) {
                copiedTemplateArguments.push_back(templateArgument->deepCopy());
            }

            return new TemplatedType(_qualifier, _matchingTemplateDecls, copiedTemplateArguments,
                                     _startPosition, _endPosition);
        }

        ~TemplatedType() override {
            for (Expr* templateArgument : _templateArguments) {
                delete templateArgument;
            }
        }

    protected:
        // We store a list of declarations that partially match the template parameters
        // (basically these have template parameter lists with the same number of required parameters as there are
        // parameters provided)
        std::vector<Decl*> _matchingTemplateDecls;
        std::vector<Expr*> _templateArguments;
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_TEMPLATEDTYPE_HPP
