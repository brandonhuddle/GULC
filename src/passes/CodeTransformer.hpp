#ifndef GULC_CODETRANSFORMER_HPP
#define GULC_CODETRANSFORMER_HPP

namespace gulc {
    /**
     * CodeTransformer handles the following:
     *  * Verify all Stmts and Exprs are valid
     *  * Add temporary values to the AST
     *  * Insert destructor calls and other deferred statements
     *
     * The purpose of this is to validate the code that has come in from the source file (mostly unchanged since
     * parsing) then it transforms the code into something much easier for `CodeGen` to work with.
     */
    class CodeTransformer {

    };
}

#endif //GULC_CODETRANSFORMER_HPP
