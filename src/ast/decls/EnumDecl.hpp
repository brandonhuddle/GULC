#ifndef GULC_ENUMDECL_HPP
#define GULC_ENUMDECL_HPP

#include <ast/Decl.hpp>
#include <llvm/Support/Casting.h>
#include <ast/Type.hpp>
#include "EnumConstDecl.hpp"

namespace gulc {
    /**
     * Enums in GUL are similar to C# enums. They are a list of named `const` values, an enum type can extend types
     * (`: type`) to set the type of the `const` values, etc.
     *
     * One small difference is GUL enum base types can be ANY `const` type, not just primitives.
     * Example:
     *
     *     enum StringExample: std.string {
     *         Paint = "paint",
     *         Copy = "copy",
     *         // ...
     *     }
     *
     * Since `std.string` is `const` it can be used as the enum base. We might at some point make this more strict
     *
     * NOTE: We might add `enum class` or more accurately `enum union` to handle the Rust/Swift style enums.
     */
    class EnumDecl : public Decl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::Enum; }

        // This is the type of all `const` values
        // NOTE: If no `constType` is defined this will default to `i32`
        Type* constType;

        EnumDecl(unsigned int sourceFileID, std::vector<Attr*> attributes, Identifier identifier,
                 TextPosition startPosition, TextPosition endPosition, Type* constType,
                 std::vector<EnumConstDecl*> enumConsts)
                : Decl(Kind::Enum, sourceFileID, std::move(attributes),
                       Decl::Visibility::Unassigned, false, std::move(identifier),
                       DeclModifiers::None),
                  _startPosition(startPosition), _endPosition(endPosition), constType(constType),
                  _enumConsts(std::move(enumConsts)) {}

        std::vector<EnumConstDecl*>& enumConsts() { return _enumConsts; }
        std::vector<EnumConstDecl*> const& enumConsts() const { return _enumConsts; }

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Decl* deepCopy() const override {
            std::vector<Attr*> copiedAttributes;
            copiedAttributes.reserve(_attributes.size());
            std::vector<EnumConstDecl*> copiedEnumConsts;
            copiedEnumConsts.reserve(_enumConsts.size());
            Type* copiedConstType = nullptr;

            for (Attr* attribute : _attributes) {
                copiedAttributes.push_back(attribute->deepCopy());
            }

            for (EnumConstDecl* enumConst : _enumConsts) {
                copiedEnumConsts.push_back(llvm::dyn_cast<EnumConstDecl>(enumConst->deepCopy()));
            }

            if (constType != nullptr) {
                copiedConstType = constType->deepCopy();
            }

            return new EnumDecl(_sourceFileID, copiedAttributes, _identifier,
                                _startPosition, _endPosition, copiedConstType, copiedEnumConsts);
        }

        ~EnumDecl() override {
            delete constType;

            for (EnumConstDecl* enumConst : _enumConsts) {
                delete enumConst;
            }
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;
        std::vector<EnumConstDecl*> _enumConsts;

    };
}

#endif //GULC_ENUMDECL_HPP
