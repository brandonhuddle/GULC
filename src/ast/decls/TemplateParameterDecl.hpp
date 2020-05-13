#ifndef GULC_TEMPLATEPARAMETERDECL_HPP
#define GULC_TEMPLATEPARAMETERDECL_HPP

#include <ast/Decl.hpp>
#include <ast/Type.hpp>

namespace gulc {
    class TemplateParameterDecl : public Decl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::TemplateParameter; }

        enum class TemplateParameterKind {
            // A type `struct Example<T> == Example<int>`
            Typename,
            // A const variable `struct Example<const param: int> == Example<12>`
            Const
        };

        // This is null if the kind is `typename`
        Type* constType;
        TemplateParameterKind templateParameterKind() const { return _templateParameterKind; }

        // Constructor for `typename`
        TemplateParameterDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Identifier identifier,
                              TextPosition startPosition, TextPosition endPosition)
                : Decl(Decl::Kind::TemplateParameter, sourceFileID, std::move(attributes),
                       Decl::Visibility::Unassigned, true, std::move(identifier),
                       DeclModifiers::None),
                  constType(nullptr), _templateParameterKind(TemplateParameterKind::Typename),
                  _startPosition(startPosition), _endPosition(endPosition) {}
        // Constructor for `const`
        TemplateParameterDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Identifier identifier,
                              Type* type, TextPosition startPosition, TextPosition endPosition)
                : Decl(Decl::Kind::TemplateParameter, sourceFileID, std::move(attributes),
                       Decl::Visibility::Unassigned, true, std::move(identifier),
                       DeclModifiers::None),
                  constType(type), _templateParameterKind(TemplateParameterKind::Const),
                  _startPosition(startPosition), _endPosition(endPosition) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Decl* deepCopy() const override {
            std::vector<Attr*> copiedAttributes;
            copiedAttributes.reserve(_attributes.size());
            std::vector<Type*> copiedInheritedTypes;
            copiedInheritedTypes.reserve(inheritedTypes.size());

            for (Attr* attribute : _attributes) {
                copiedAttributes.push_back(attribute->deepCopy());
            }

            for (Type* inheritedType : inheritedTypes) {
                copiedInheritedTypes.push_back(inheritedType->deepCopy());
            }

            TemplateParameterDecl* result = nullptr;

            if (_templateParameterKind == TemplateParameterKind::Const) {
                result = new TemplateParameterDecl(_sourceFileID, copiedAttributes, _identifier,
                                                   constType->deepCopy(), _startPosition, _endPosition);
            } else {
                result = new TemplateParameterDecl(_sourceFileID, copiedAttributes, _identifier,
                                                   _startPosition, _endPosition);
            }

            result->container = container;
            result->containedInTemplate = containedInTemplate;
            result->inheritedTypes = copiedInheritedTypes;
            return result;
        }

        ~TemplateParameterDecl() override {
            delete constType;

            for (Type* inheritedType : inheritedTypes) {
                delete inheritedType;
            }
        }

        // These will be defined by `where` contracts
        // NOTE: This will not apply to `const` template parameters
        std::vector<Type*> inheritedTypes;
        // TODO: We'll also have to define `members` and probably `constructors`
        //       We shouldn't define a destructor as that should be implied (ALL types have destructors,
        //       some just can be deleted implicitly. e.g. `i32` "has" a destructor but it is worthless and never even
        //       considered)

    protected:
        TemplateParameterKind _templateParameterKind;
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_TEMPLATEPARAMETERDECL_HPP
