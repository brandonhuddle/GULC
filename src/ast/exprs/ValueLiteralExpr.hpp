#ifndef GULC_VALUELITERALEXPR_HPP
#define GULC_VALUELITERALEXPR_HPP

#include <ast/Expr.hpp>
#include <string>
#include <ast/Type.hpp>

namespace gulc {
    class ValueLiteralExpr : public Expr {
    public:
        static bool classof(const Expr* expr) { return expr->getExprKind() == Expr::Kind::ValueLiteral; }

        enum class LiteralType {
            Char,
            Float,
            Integer,
            String
        };

        /// What kind of literal this value is
        LiteralType literalType() const { return _literalType; }
        /// The string representation for the provided integer literal, this might be larger than can fit into the
        /// literal type
        std::string const& value() const { return _value; }
        /// The provided suffix for the integer literal. I.e. `12i32`, `"example"str`, `2000usize`, `1.0f32`
        std::string const& suffix() const { return _suffix; }
        /// Whether a suffix was provided or not
        bool hasSuffix() const { return !_suffix.empty(); }

        ValueLiteralExpr(LiteralType literalType, std::string value, std::string suffix,
                         TextPosition startPosition, TextPosition endPosition)
                : Expr(Expr::Kind::ValueLiteral),
                  _literalType(literalType), _value(std::move(value)), _suffix(std::move(suffix)),
                  _startPosition(startPosition), _endPosition(endPosition) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        Expr* deepCopy() const override {
            ValueLiteralExpr* result = new ValueLiteralExpr(_literalType, _value, _suffix,
                                                           _startPosition, _endPosition);

            if (valueType != nullptr) {
                result->valueType = valueType->deepCopy();
            }

            return result;
        }

        std::string toString() const override {
            return _value + _suffix;
        }

    protected:
        LiteralType _literalType;
        std::string _value;
        std::string _suffix;
        // NOTE: There CANNOT be a space between the number and the suffix so the suffix start and end can be
        //       calculated using the length of the value
        TextPosition _startPosition;
        TextPosition _endPosition;

    };
}

#endif //GULC_VALUELITERALEXPR_HPP
