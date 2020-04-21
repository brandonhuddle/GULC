#ifndef GULC_ENSURESCONT_HPP
#define GULC_ENSURESCONT_HPP

#include <ast/Cont.hpp>
#include <ast/Expr.hpp>

namespace gulc {
    class EnsuresCont : public Cont {
    public:
        static bool classof(const Cont* cont) { return cont->getContKind() == Cont::Kind::Ensures; }

        Expr* condition;

        EnsuresCont(Expr* condition, TextPosition startPosition, TextPosition endPosition)
                : Cont(Cont::Kind::Ensures, startPosition, endPosition), condition(condition) {}

        Cont* deepCopy() const override {
            return new EnsuresCont(condition->deepCopy(), _startPosition, _endPosition);
        }

        ~EnsuresCont() override {
            delete condition;
        }

    };
}

#endif //GULC_ENSURESCONT_HPP
