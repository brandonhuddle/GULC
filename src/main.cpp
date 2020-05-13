#include <parsing/Parser.hpp>
#include <passes/BasicTypeResolver.hpp>
#include <passes/DeclInstantiator.hpp>
#include <passes/NamespacePrototyper.hpp>
#include <passes/BasicDeclValidator.hpp>
#include "Target.hpp"

using namespace gulc;

// TODO: Should we add a `toString()` function to `Expr`?
//         1. It would be useful for error messages AND only used for error messages
//         2. AFAIK everything within an `Expr` is easily 1:1 translatable to a string representation?
//         3. It is really needed for error messages. `Example<...>` looks awful.

// TODO: AAAHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH Template Contracts!
//        3. Attempt to detect templates that are identical with identical contracts
//        5. Use `where T has <member signature>` to give `TemplateParameter`s type information

// TODO: For `where` contracts we should do:
//        2. For `const`: allow `>`, `<`, `==`, `!=`, `<=`, `>=`, and all math operators
//        3. Disallow all boolean operators EXCEPT `||`
//        4. Require the `const` to be on the left side (i.e. ONLY `where T == 12` NOT `where `12 == T`!)
//        5. Disallow parenthesis (no `where (T == 12)` EXCEPT for `where T has (func T())` that is needed for nested contracts)

// TODO: To properly handle contracts in ALL situations we MIGHT need to changed `NestedType` to `DependantType`
//       This is because we will need an EXACT `Decl` to handle `where T : G`, which `NestedType` doesn't give us.
//       The way `DependantType` would work is EXACTLY the same as `NestedType` EXCEPT instead of working like
//       `UnresolvedType` it will contain ANOTHER `Type` that is the `Dependant`

int main() {
    Target target = Target::getHostTarget();

    std::vector<std::string> filePaths {
            "examples/TestFile.gul"
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
    BasicDeclValidator basicDeclValidator(filePaths, prototypes);
    basicDeclValidator.processFiles(parsedFiles);

    // Resolve all types as much as possible, leaving `TemplatedType`s for any templates
    BasicTypeResolver basicTypeResolver(filePaths, prototypes);
    basicTypeResolver.processFiles(parsedFiles);

    // Instantiate Decl instances as much as possible (set `StructDecl` data layouts, instantiate `TemplatedType`, etc.)
    DeclInstantiator declInstantiator(target, filePaths);
    declInstantiator.processFiles(parsedFiles);

    return 0;
}
