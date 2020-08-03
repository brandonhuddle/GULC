#ifndef GULC_CONSTRUCTORCALLEXPR_HPP
#define GULC_CONSTRUCTORCALLEXPR_HPP

#include <vector>
#include <ast/exprs/FunctionCallExpr.hpp>
#include <llvm/Support/Casting.h>
#include <ast/decls/ConstructorDecl.hpp>
#include "LabeledArgumentExpr.hpp"
#include "ConstructorReferenceExpr.hpp"

namespace gulc {
    class ConstructorCallExpr : public FunctionCallExpr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::ConstructorCall; }

        ConstructorCallExpr(ConstructorReferenceExpr* constructorReference, std::vector<LabeledArgumentExpr*> arguments,
                            TextPosition startPosition, TextPosition endPosition)
                : FunctionCallExpr(Expr::Kind::ConstructorCall, constructorReference, std::move(arguments),
                                   startPosition, endPosition) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Expr* deepCopy() const override {
            std::vector<LabeledArgumentExpr*> copiedArguments;
            copiedArguments.reserve(arguments.size());
            ConstructorReferenceExpr* copiedConstructorReference = nullptr;

            for (Expr* argument : arguments) {
                copiedArguments.push_back(llvm::dyn_cast<LabeledArgumentExpr>(argument->deepCopy()));
            }

            if (functionReference != nullptr && llvm::isa<ConstructorReferenceExpr>(functionReference)) {
                copiedConstructorReference = llvm::dyn_cast<ConstructorReferenceExpr>(functionReference->deepCopy());
            }

            auto result = new ConstructorCallExpr(copiedConstructorReference, copiedArguments,
                                                  _startPosition, _endPosition);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            std::string argumentsString;

            for (std::size_t i = 0; i < arguments.size(); ++i) {
                if (i != 0) argumentsString += ", ";

                argumentsString += arguments[i]->toString();
            }

            return functionReference->toString() + "(" + argumentsString + ")";
        }

    protected:
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_CONSTRUCTORCALLEXPR_HPP
