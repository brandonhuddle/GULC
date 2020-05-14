#ifndef GULC_PREFIXOPERATOREXPR_HPP
#define GULC_PREFIXOPERATOREXPR_HPP

#include <ast/Expr.hpp>

namespace gulc {
    enum class PrefixOperators {
        Increment, // ++
        Decrement, // --

        Positive, // +
        Negative, // -

        LogicalNot, // !
        BitwiseNot, // ~

        Dereference, // *
        Reference, // &

        // Get the size of any type
        SizeOf, // sizeof
        // Get the alignment of any type OR member
        AlignOf, // alignof
        // Get the offset of an member in a struct/class
        OffsetOf, // offsetof
        // Get the name of any decl (e.g. `nameof(std.io.Stream) == "Stream"`, `nameof(ExampleClass::Member) == "Member")
        // This is useful for making refactoring easier. Sometimes you need to know the name of something but if you
        // use the exact string value of the name it might not get recognized in refactoring. Making getting the name a
        // part of the actual language will trigger an error if the decl within the `nameof` wasn't found.
        NameOf, // nameof
        // Get known traits of a type
        TraitsOf, // traitsof
    };

    inline std::string getPrefixOperatorString(PrefixOperators prefixOperator) {
        switch (prefixOperator) {
            case PrefixOperators::Increment:
                return "++";
            case PrefixOperators::Decrement:
                return "--";
            case PrefixOperators::Positive:
                return "+";
            case PrefixOperators::Negative:
                return "-";
            case PrefixOperators::LogicalNot:
                return "!";
            case PrefixOperators::BitwiseNot:
                return "~";
            case PrefixOperators::Dereference:
                return "*";
            case PrefixOperators::Reference:
                return "&";
            case PrefixOperators::SizeOf:
                return "sizeof";
            case PrefixOperators::AlignOf:
                return "alignof";
            case PrefixOperators::OffsetOf:
                return "offsetof";
            case PrefixOperators::NameOf:
                return "nameof";
            case PrefixOperators::TraitsOf:
                return "traitsof";
            default:
                return "[UNKNOWN]";
        }
    }

    class PrefixOperatorExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::PrefixOperator; }

        Expr* nestedExpr;

        PrefixOperators prefixOperator() const { return _prefixOperator; }

        PrefixOperatorExpr(PrefixOperators prefixOperator, Expr* nestedExpr,
                           TextPosition operatorStartPosition, TextPosition operatorEndPosition)
                : Expr(Expr::Kind::PrefixOperator),
                  nestedExpr(nestedExpr), _operatorStartPosition(operatorStartPosition),
                  _operatorEndPosition(operatorEndPosition), _prefixOperator(prefixOperator) {}

        TextPosition startPosition() const override { return _operatorStartPosition; }
        TextPosition endPosition() const override { return nestedExpr->endPosition(); }

        Expr* deepCopy() const override {
            return new PrefixOperatorExpr(_prefixOperator, nestedExpr->deepCopy(),
                                          _operatorStartPosition, _operatorEndPosition);
        }

        std::string toString() const override {
            return getPrefixOperatorString(_prefixOperator) + "(" + nestedExpr->toString() + ")";
        }

        ~PrefixOperatorExpr() override {
            delete nestedExpr;
        }

    protected:
        TextPosition _operatorStartPosition;
        TextPosition _operatorEndPosition;
        PrefixOperators _prefixOperator;

    };
}

#endif //GULC_PREFIXOPERATOREXPR_HPP
