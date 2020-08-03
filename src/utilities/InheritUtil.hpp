#ifndef GULC_INHERITUTIL_HPP
#define GULC_INHERITUTIL_HPP

#include <ast/Decl.hpp>

namespace gulc {
    class InheritUtil {
    public:
        /// Checks if `checkOverrides` either shadows or overrides `checkAgainst`
        static bool overridesOrShadows(Decl* checkOverrides, Decl* checkAgainst);
    };
}

#endif //GULC_INHERITUTIL_HPP
