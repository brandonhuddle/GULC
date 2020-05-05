#ifndef GULC_CONSTEXPRHELPER_HPP
#define GULC_CONSTEXPRHELPER_HPP

#include <ast/Expr.hpp>
#include <ast/Type.hpp>
#include <vector>

namespace gulc {
    class ConstExprHelper {
    public:
        static bool compareAreSame(Expr const* left, Expr const* right);
        static bool templateArgumentsAreSolved(std::vector<Expr*>& templateArguments);

    private:
        static bool templateArgumentIsSolved(Expr* checkArgument);
        static bool templateTypeArgumentIsSolved(Type* checkType);

    };
}

#endif //GULC_CONSTEXPRHELPER_HPP
