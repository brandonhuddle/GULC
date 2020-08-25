#ifndef GULC_STORETEMPORARYVALUEEXPR_HPP
#define GULC_STORETEMPORARYVALUEEXPR_HPP

#include <ast/Expr.hpp>
#include <llvm/Support/Casting.h>
#include "TemporaryValueRefExpr.hpp"

namespace gulc {
    // This class exists for the sole purpose of supporting `sret` parameters since LLVM can't just handle the ABI
    // issues for us. So we now always return structs as the first parameter of a function.
    class StoreTemporaryValueExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::StoreTemporaryValue; }

        TemporaryValueRefExpr* temporaryValue;
        Expr* storeValue;

        StoreTemporaryValueExpr(TemporaryValueRefExpr* temporaryValue, Expr* storeValue,
                                TextPosition startPosition, TextPosition endPosition)
                : Expr(Expr::Kind::StoreTemporaryValue),
                  temporaryValue(temporaryValue), storeValue(storeValue), _startPosition(startPosition),
                  _endPosition(endPosition) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Expr* deepCopy() const override {
            auto result = new StoreTemporaryValueExpr(
                    llvm::dyn_cast<TemporaryValueRefExpr>(temporaryValue->deepCopy()),
                    storeValue->deepCopy(),
                    _startPosition,
                    _endPosition
            );
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return storeValue->toString();
        }

        ~StoreTemporaryValueExpr() override {
            delete temporaryValue;
            delete storeValue;
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_STORETEMPORARYVALUEEXPR_HPP
