#ifndef GULC_FUNCTORUTIL_HPP
#define GULC_FUNCTORUTIL_HPP

#include "SignatureComparer.hpp"

namespace gulc {
    /**
     * `FunctorUtil` is used to check types for "functor" abilities.
     * The below are valid functors:
     *
     *     var funcPointer: func(param1: i32)
     *
     *     struct CallExample {
     *         call(param1: i32) {}
     *     }
     *
     *     var callExample = CallExample.init()
     *     callExample()
     */
    class FunctorUtil {
    public:
        /**
         * Check if the `checkType` is valid to be called as a functor.
         *
         * @param checkType
         * @param arguments
         * @param outFoundDecl - Filled with the found `Decl` underlying the functor call (only for Struct and Trait)
         * @return
         */
        static SignatureComparer::ArgMatchResult checkValidFunctorCall(Type* checkType,
                std::vector<LabeledArgumentExpr*> const& arguments, Decl** outFoundDecl);

    };
}

#endif //GULC_FUNCTORUTIL_HPP
