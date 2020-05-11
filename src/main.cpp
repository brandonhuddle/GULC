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

// TODO: We need to finish `DeclInstantiator` for templates. We should now be able to properly handle the instantiator
//       using the new `NestedType`, `TemplateStructType`, and `TemplateTraitType`
//        1. Create a way to create type information about template typenames through contract `where`
//        2. Use this new information to resolve templates to `Template*Type`
//        3. ???

// TODO: We might need to do a prepass to handle processing template `contracts` before `DeclInstantiator`
//       Without doing this it will be nearly impossible to properly handle template types.
//        1. If one of the possible templates extends a type that references a template type with the same name but
//           different contracts then that will trigger an error since we will have to instantiate the template that
//           extends the type to see if it is the correct template. So even though it wasn't the template being
//           referenced by the programmer we're still erroring out for something we shouldn't error out for.
//        2. ALTERNATIVE: We MIGHT be able to circumvent this issue by making a new `instantiateTemplate*Contracts`
//           function and adding a `contractsAreInstantiated` member to all template decls. This would make it so
//           we can instantiate and test contracts without having to handle the rest of the template
//        3. We CANNOT do the above because that still brings the issue of some templates depending on the type being
//           used as a parameter. We HAVE to do a prepass
//        4. AHHHHHHHHHHHHHHHHHHHHHHHH Maybe I'm just tired but I believe we actually CAN do the above, with ONLY the
//           instantiateContracts shit.

// TODO: AAAHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHHH Template Contracts!
//        1. Allow `where T : <type>`
//        2. If the template arguments pass for two templates with the same signature we error saying they are
//           ambiguous
//        3. Attempt to detect templates that are identical with identical contracts
//        4. Use `where T : <type>` to give `TemplateParameter`s type information
//        5. Use `where T has <member signature>` to give `TemplateParameter`s type information

// TODO: For `where` contracts we should do:
//        1. Same as above with `where T : <type>` for types
//        2. For `const`: allow `>`, `<`, `==`, `!=`, `<=`, `>=`, and all math operators
//        3. Disallow all boolean operators EXCEPT `||`
//        4. Require the `const` to be on the left side (i.e. ONLY `where T == 12` NOT `where `12 == T`!)
//        5. Disallow parenthesis (no `where (T == 12)` EXCEPT for `where T has (func T())` that is needed for nested contracts)

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
