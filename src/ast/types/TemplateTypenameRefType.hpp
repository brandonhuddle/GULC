#ifndef GULC_TEMPLATETYPENAMEREFTYPE_HPP
#define GULC_TEMPLATETYPENAMEREFTYPE_HPP

#include <ast/Type.hpp>
#include <ast/decls/TemplateParameterDecl.hpp>

namespace gulc {
    class TemplateTypenameRefType : public Type {
    public:
        static bool classof(const Type* type) { return type->getTypeKind() == Type::Kind::TemplateTypenameRef; }

        TemplateTypenameRefType(Qualifier qualifier, TemplateParameterDecl* refTemplateParameter,
                                TextPosition startPosition, TextPosition endPosition)
                : Type(Type::Kind::TemplateTypenameRef, qualifier, false),
                  _refTemplateParameter(refTemplateParameter),
                  _startPosition(startPosition), _endPosition(endPosition) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        TemplateParameterDecl* refTemplateParameter() const { return _refTemplateParameter; }

        std::string toString() const override {
            return "&<" + _refTemplateParameter->identifier().name() + ">";
        }

    protected:
        TemplateParameterDecl* _refTemplateParameter;
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_TEMPLATETYPENAMEREFTYPE_HPP
