#ifndef GULC_REQUIRESCONT_HPP
#define GULC_REQUIRESCONT_HPP

#include <ast/Cont.hpp>
#include <ast/Expr.hpp>

namespace gulc {
    class RequiresCont : public Cont {
    public:
        static bool classof(const Cont* cont) { return cont->getContKind() == Cont::Kind::Requires; }

        Expr* condition;

        RequiresCont(Expr* condition, TextPosition startPosition, TextPosition endPosition)
                : Cont(Cont::Kind::Requires, startPosition, endPosition), condition(condition) {}

        ~RequiresCont() override {
            delete condition;
        }

    };
}

#endif //GULC_REQUIRESCONT_HPP
