#ifndef GULC_WHERECONT_HPP
#define GULC_WHERECONT_HPP

#include <ast/Cont.hpp>
#include <ast/Expr.hpp>

namespace gulc {
    /**
     * WhereCont is used to change how templates are used. It is basically `Requires` but for template types and
     * template constants
     */
    class WhereCont : public Cont {
    public:
        static bool classof(const Cont* cont) { return cont->getContKind() == Cont::Kind::Where; }

        Expr* condition;

        WhereCont(Expr* condition, TextPosition startPosition, TextPosition endPosition)
                : Cont(Cont::Kind::Where, startPosition, endPosition), condition(condition) {}

        ~WhereCont() override {
            delete condition;
        }

    };
}

#endif //GULC_WHERECONT_HPP
