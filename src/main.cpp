#include <parsing/Parser.hpp>
#include <passes/BasicTypeResolver.hpp>
#include <passes/DeclInstantiator.hpp>
#include <passes/NamespacePrototyper.hpp>
#include <passes/BasicDeclValidator.hpp>
#include "Target.hpp"

using namespace gulc;

// TODO: There are currently issues with how we handle templates
//        1. We can't run them through the `DeclInstantiator` because we need to keep `TemplatedTypes`
//        2. Validation is handled on their instantiated forms rather than the templated form (causing confusing errors like C++ has, all errors should be triggered on the template NOT the instantiation)
//        3. There are issues with referencing types nested within templated declarations (a struct nested within a template struct is NOT being properly referenced, there should be unique structs per each template instantiation. This isn't being done)
//       To fix these issues I believe we should do the following:
//        1. Create a `PartialType` to denote a `TemplatedType` that has been validated and resolved but NOT instantiated
//        2. Either extend `PartialType` to support it OR create a new `TemplatedNestedType` to handle types nested within templated types
//        3. Within `DeclInstantiator`, use the `where` contracts on templates to construct a `ContractualType` which will be used for validating a template

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

    // Validate imports, check for obvious redefinitions, etc.
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
