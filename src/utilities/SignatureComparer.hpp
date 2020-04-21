#ifndef GULC_SIGNATURECOMPARER_HPP
#define GULC_SIGNATURECOMPARER_HPP

#include <ast/decls/FunctionDecl.hpp>

namespace gulc {
    class SignatureComparer {
    public:
        enum CompareResult {
            Different,
            /// Similar means the functions could be called with the same arguments but are differentiable
            Similar,
            /// Names are exactly the same, parameter types are exactly the same, and parameter names are the same
            Exact
        };

        /**
         * Compare two `FunctionDecl`s based on the function name, parameter type, and parameter names
         *
         * @param left
         * @param right
         * @param checkSimilar When true we will check if the functions are similar, if false we fail at first difference
         * @return
         */
        static CompareResult compareFunctions(FunctionDecl const* left, FunctionDecl const* right,
                                              bool checkSimilar = true);

    };
}

#endif //GULC_SIGNATURECOMPARER_HPP
