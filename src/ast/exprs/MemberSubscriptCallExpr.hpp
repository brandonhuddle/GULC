#ifndef GULC_MEMBERSUBSCRIPTCALLEXPR_HPP
#define GULC_MEMBERSUBSCRIPTCALLEXPR_HPP

#include "SubscriptCallExpr.hpp"

namespace gulc {
    class MemberSubscriptCallExpr : public SubscriptCallExpr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::MemberSubscriptCall; }

        // This could be either `SelfRefExpr` OR an `Expr` that returns a struct type.
        Expr* selfArgument;

        MemberSubscriptCallExpr(Expr* subscriptReference, Expr* selfArgument,
                                std::vector<LabeledArgumentExpr*> arguments,
                                TextPosition startPosition, TextPosition endPosition)
                : SubscriptCallExpr(Expr::Kind::MemberSubscriptCall, subscriptReference, std::move(arguments),
                                    startPosition, endPosition),
                  selfArgument(selfArgument) {}

        Expr* deepCopy() const override {
            std::vector<LabeledArgumentExpr*> copiedArguments;
            copiedArguments.reserve(arguments.size());
            Expr* copiedSelfArgument = nullptr;

            for (Expr* argument : arguments) {
                copiedArguments.push_back(llvm::dyn_cast<LabeledArgumentExpr>(argument->deepCopy()));
            }

            if (selfArgument != nullptr) {
                copiedSelfArgument = selfArgument->deepCopy();
            }

            auto result = new MemberSubscriptCallExpr(subscriptReference->deepCopy(),
                                                     copiedSelfArgument, copiedArguments,
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

            std::string selfString;

            if (selfArgument != nullptr) {
                selfString = selfArgument->toString() + ".";
            }

            return selfString + subscriptReference->toString() + "[" + argumentsString + "]";
        }

        ~MemberSubscriptCallExpr() override {
            delete selfArgument;
        }

    };
}

#endif //GULC_MEMBERSUBSCRIPTCALLEXPR_HPP
