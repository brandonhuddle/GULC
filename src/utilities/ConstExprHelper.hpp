#ifndef GULC_CONSTEXPRHELPER_HPP
#define GULC_CONSTEXPRHELPER_HPP

#include <ast/Expr.hpp>

namespace gulc {
    class ConstExprHelper {
    public:
        static bool compareAreSame(Expr const* left, Expr const* right);

    };
}

#endif //GULC_CONSTEXPRHELPER_HPP
