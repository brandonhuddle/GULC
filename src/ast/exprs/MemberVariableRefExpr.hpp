#ifndef GULC_MEMBERVARIABLEREFEXPR_HPP
#define GULC_MEMBERVARIABLEREFEXPR_HPP

#include <ast/Expr.hpp>
#include <ast/decls/VariableDecl.hpp>
#include <ast/types/StructType.hpp>

namespace gulc {
    class MemberVariableRefExpr : public Expr {
    public:
        static bool classof(const Expr *expr) { return expr->getExprKind() == Kind::MemberVariableRef; }

        // `self`, `obj`, `*somePtr`
        Expr* object;
        // NOTE: Only `StructDecl` can contain member variables
        StructType* structType;

        MemberVariableRefExpr(TextPosition startPosition, TextPosition endPosition,
                              Expr* object, StructType* structType, gulc::VariableDecl* variableDecl)
                : Expr(Expr::Kind::MemberVariableRef),
                  _startPosition(startPosition), _endPosition(endPosition),
                  object(object), structType(structType), _variableDecl(variableDecl) {}

        TextPosition startPosition() const override { return _startPosition; }
        TextPosition endPosition() const override { return _endPosition; }

        gulc::VariableDecl* variableDecl() const { return _variableDecl; }

        Expr* deepCopy() const override {
            auto result = new MemberVariableRefExpr(_startPosition, _endPosition,
                                                    object->deepCopy(),
                                                    llvm::dyn_cast<StructType>(structType->deepCopy()),
                                                    _variableDecl);
            result->valueType = valueType == nullptr ? nullptr : valueType->deepCopy();
            return result;
        }

        std::string toString() const override {
            return object->toString() + "." + _variableDecl->identifier().name();
        }

        ~MemberVariableRefExpr() override {
            delete object;
            delete structType;
        }

    private:
        TextPosition _startPosition;
        TextPosition _endPosition;
        gulc::VariableDecl* _variableDecl;

    };
}

#endif //GULC_MEMBERVARIABLEREFEXPR_HPP
