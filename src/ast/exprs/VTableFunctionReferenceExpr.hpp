#ifndef GULC_VTABLEFUNCTIONREFERENCEEXPR_HPP
#define GULC_VTABLEFUNCTIONREFERENCEEXPR_HPP

#include <ast/Expr.hpp>
#include <ast/decls/StructDecl.hpp>

namespace gulc {
    /**
     * `VTableFunctionReferenceExpr` is used for storing only vtable function references
     *
     * Examples:
     *     structVar.virtualFunc(...)
     *     virtualFunc(...) // Implicit self
     *
     * NOTE: `Struct::virtualFunc` would NOT be a vtable function reference, it would be a static
     *       `FunctionReferenceExpr`
     */
    class VTableFunctionReferenceExpr : public Expr {
    public:
        static bool classof(const Expr *expr) { return expr->getExprKind() == Kind::VTableFunctionReference; }

        VTableFunctionReferenceExpr(TextPosition startPosition, TextPosition endPosition,
                                    StructDecl* structDecl, std::size_t vtableIndex)
                : Expr(Expr::Kind::VTableFunctionReference),
                  _startPosition(startPosition), _endPosition(endPosition),
                  _structDecl(structDecl), _vtableIndex(vtableIndex) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Expr* deepCopy() const override {
            auto result = new VTableFunctionReferenceExpr(_startPosition, _endPosition,
                                                          _structDecl, _vtableIndex);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return _structDecl->vtable[_vtableIndex]->identifier().name();
        }

    private:
        TextPosition _startPosition;
        TextPosition _endPosition;
        /// The struct for the vtable (NOTE: this is only the struct for the KNOWN type, the actual type may be
        /// different, this should only be used for basic sanity checks and output, NOT inlining)
        StructDecl* _structDecl;
        /// The actual index within the vtable this function was found at.
        std::size_t _vtableIndex;

    };
}

#endif //GULC_VTABLEFUNCTIONREFERENCEEXPR_HPP
