#include <parsing/Parser.hpp>
#include <passes/BasicTypeResolver.hpp>
#include <passes/DeclInstantiator.hpp>
#include <passes/NamespacePrototyper.hpp>
#include <passes/BasicDeclValidator.hpp>
#include <passes/CodeProcessor.hpp>
#include "Target.hpp"

using namespace gulc;

// TODO: We need an alternative to `isize` and `usize`. While they are good for pointers they are NOT good for IO.
//       With IO we might be working with files larger than can be indexed with `isize` and `usize`. Need to account for
//       this with maybe an `fsize`, `osize`, `ioffset`, `uoffset`, or something else. Or maybe just make sure to use
//       `u64` in these scenarios.

// TODO: Next features to work on:
//       * [POSTPONED] Finish adding support for extensions...
//       * Improve error checking for Decls post-instantiation
//       * Rework the passes to now handle Stmt and Expr as early as possible (doing something similar to
//         `TemplatedType` and `DependentType` with function calls)
//       * Improve `const` solving (and simplify expressions as much as possible in trivial situations like `i + 2 + 2` == `i + 4`)
//       * Output working executables through IR generation... sounds so quick right? Well hopefully it will be since we already have that from last years compiler...
//       * `peekStartPosition` needs to handle removing any whitespace. Currently doing `peekStartPosition` for statements within a function will return `0, 0` for properly formatted code.

// TODO: I think for processing `Expr` and `Stmt` we should do this:
//       1. Process `Stmt` within the type resolver
//       2. Generate lists of potential functions for `FunctionCallExpr`, `IndexerCallExpr`, etc. similar to
//          `TemplatedType`
//  [NOTE]: I don't think we should actually do the above. We should wait until `CodeProcessor` and just do a manual
//          search that fills lists, just handle everything all at once.
//       3. Attempt to handle as much as possible before `CodeProcessor`
//       4. ALL `const` EXPRESSIONS SHOULD BE HANDLED BEFORE `CodeProcessor` (i.e. we should finalize type resolution
//          for hard `Type` instances within `DeclInstantiator`, would shouldn't be touching the `Type` for
//          `VariableDeclExpr` for instance as we know it is a type and we should be able to solve it within
//          `DeclInstantiator`)

// TODO: Sidenotes:
//       * `ghoul` should handle creating new projects for us (i.e. `ghoul new Example`, `ghoul new --template=[URI] Example`)
//       * `ghoul` should handle the build files (e.g. replacement for `cmake`, should be able to also handle calling
//         the C compiler and other language compilers)
//       * Basically `ghoul` should be a compiler, a build system, and a project creation tool

// TODO: We need to add `inheritedMembers` to both `StructDecl` and `TraitDecl`, we need quick access to the inherited
//       members and due to so many members being able to come from multiple types (and those types having members that
//       can come from even more types and so on) we need to just fill `inheritedMembers`

int main() {
    Target target = Target::getHostTarget();

    std::vector<std::string> filePaths {
            "examples/TestFile.ghoul"
//            "examples/TemplateWhereContractTest.ghoul"
    };
    std::vector<ASTFile> parsedFiles;

    for (std::size_t i = 0; i < filePaths.size(); ++i) {
        Parser parser;
        parsedFiles.push_back(parser.parseFile(i, filePaths[i]));
    }

    // Generate namespace map
    NamespacePrototyper namespacePrototyper;
    std::vector<NamespaceDecl*> prototypes = namespacePrototyper.generatePrototypes(parsedFiles);

    // Validate imports, check for obvious redefinitions, set the `Decl::container` member, etc.
    // TODO: Should we rename this to `BasicValidator` and move the label validation to here?
    //       I would go ahead and do it but I'm not sure if the overhead is worth it, I think it would be better
    //       to wait until we're doing more processing of the `Stmt`s...
    BasicDeclValidator basicDeclValidator(filePaths, prototypes);
    basicDeclValidator.processFiles(parsedFiles);

    // Resolve all types as much as possible, leaving `TemplatedType`s for any templates
    BasicTypeResolver basicTypeResolver(filePaths, prototypes);
    basicTypeResolver.processFiles(parsedFiles);

    // Instantiate Decl instances as much as possible (set `StructDecl` data layouts, instantiate `TemplatedType`, etc.)
    DeclInstantiator declInstantiator(target, filePaths);
    declInstantiator.processFiles(parsedFiles);

    // Process main code before IR generation
    CodeProcessor codeProcessor(target, filePaths, prototypes);
    codeProcessor.processFiles(parsedFiles);

    return 0;
}
