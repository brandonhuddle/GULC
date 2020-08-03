#ifndef GULC_SIGNATURECOMPARER_HPP
#define GULC_SIGNATURECOMPARER_HPP

#include <ast/decls/FunctionDecl.hpp>
#include <ast/decls/TemplateParameterDecl.hpp>
#include <ast/exprs/LabeledArgumentExpr.hpp>
#include <ast/decls/TemplateFunctionDecl.hpp>
#include <ast/types/LabeledType.hpp>
#include "TypeCompareUtil.hpp"

namespace gulc {
    class SignatureComparer {
    public:
        enum CompareResult {
            Different,
            /// Similar means the functions could be called with the same parameters but are differentiable
            Similar,
            /// Names are exactly the same, parameter types are exactly the same, and parameter names are the same
            Exact
        };
        enum ArgMatchResult {
            /// No match
            Fail,
            /// The parameters are castable, they match enough that they didn't fail.
            Castable,
            /// The parameters are a perfect match for the parameters
            Match
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
        static CompareResult compareParameters(std::vector<ParameterDecl*> const& left,
                                               std::vector<ParameterDecl*> const& right,
                                               bool checkSimilar = true,
                                               TemplateComparePlan templateComparePlan =
                                                       TemplateComparePlan::CompareExact);
        static CompareResult compareTemplateFunctions(TemplateFunctionDecl const* left,
                                                      TemplateFunctionDecl const* right,
                                                      bool checkSimilar = true);
        static ArgMatchResult compareArgumentsToParameters(std::vector<ParameterDecl*> const& parameters,
                                                           std::vector<LabeledArgumentExpr*> const& arguments);
        static ArgMatchResult compareArgumentsToParameters(std::vector<ParameterDecl*> const& parameters,
                                                           std::vector<LabeledArgumentExpr*> const& arguments,
                                                           std::vector<TemplateParameterDecl*> const& templateParameters,
                                                           std::vector<Expr*> const& templateArguments);
        static ArgMatchResult compareArgumentsToParameters(std::vector<LabeledType*> const& parameters,
                                                           std::vector<LabeledArgumentExpr*> const& arguments);
        static ArgMatchResult compareTemplateArgumentsToParameters(
                std::vector<TemplateParameterDecl*> const& templateParameters,
                std::vector<Expr*> const& templateArguments);

    };
}

#endif //GULC_SIGNATURECOMPARER_HPP
