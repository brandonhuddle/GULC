#ifndef GULC_PROPERTYDECL_HPP
#define GULC_PROPERTYDECL_HPP

#include <ast/Decl.hpp>
#include <ast/Type.hpp>
#include <ast/DeclModifiers.hpp>
#include "PropertyGetDecl.hpp"
#include "PropertySetDecl.hpp"

namespace gulc {
    class PropertyDecl : public Decl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::Property; }

        // The type that will be used for the generic `get`/`set`
        Type* type;

        PropertyDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Decl::Visibility visibility,
                     bool isConstExpr, Identifier identifier, Type* type,
                     TextPosition startPosition, TextPosition endPosition, DeclModifiers declModifiers,
                     std::vector<PropertyGetDecl*> getters, PropertySetDecl* setter)
                : Decl(Decl::Kind::Property, sourceFileID, std::move(attributes), visibility, isConstExpr,
                       std::move(identifier)),
                  type(type), _startPosition(startPosition), _endPosition(endPosition),
                  _declModifiers(declModifiers), _getters(std::move(getters)), _setter(setter) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        bool isStatic() const { return (_declModifiers & DeclModifiers::Static) == DeclModifiers::Static; }
        bool isMutable() const { return (_declModifiers & DeclModifiers::Mut) == DeclModifiers::Mut; }
        bool isVolatile() const { return (_declModifiers & DeclModifiers::Volatile) == DeclModifiers::Volatile; }
        bool isAbstract() const { return (_declModifiers & DeclModifiers::Abstract) == DeclModifiers::Abstract; }
        bool isVirtual() const { return (_declModifiers & DeclModifiers::Virtual) == DeclModifiers::Virtual; }
        bool isOverride() const { return (_declModifiers & DeclModifiers::Override) == DeclModifiers::Override; }

        std::vector<PropertyGetDecl*>& getters() { return _getters; }
        std::vector<PropertyGetDecl*> const& getters() const { return _getters; }
        PropertySetDecl* setter() { return _setter; }
        bool hasSetter() const { return _setter != nullptr; }

        Decl* deepCopy() const override {
            std::vector<Attr*> copiedAttributes;
            copiedAttributes.reserve(_attributes.size());
            std::vector<PropertyGetDecl*> copiedGetters;
            copiedGetters.reserve(_getters.size());

            for (Attr* attribute : _attributes) {
                copiedAttributes.push_back(attribute->deepCopy());
            }

            for (PropertyGetDecl* getter : _getters) {
                copiedGetters.push_back(llvm::dyn_cast<PropertyGetDecl>(getter->deepCopy()));
            }

            return new PropertyDecl(_sourceFileID, copiedAttributes, _declVisibility, _isConstExpr, _identifier,
                                    type->deepCopy(), _startPosition, _endPosition,
                                    _declModifiers, copiedGetters,
                                    llvm::dyn_cast<PropertySetDecl>(_setter->deepCopy()));
        }

        ~PropertyDecl() override {
            delete type;
            delete _setter;

            for (PropertyGetDecl* getter : _getters) {
                delete getter;
            }
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;
        DeclModifiers _declModifiers;
        std::vector<PropertyGetDecl*> _getters;
        PropertySetDecl* _setter;

    };
}

#endif //GULC_PROPERTYDECL_HPP
