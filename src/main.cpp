#include <parsing/Parser.hpp>
#include <passes/TypeResolver.hpp>
#include <passes/ConstTypeResolver.hpp>
#include <passes/BaseResolver.hpp>
#include <passes/CircularReferenceChecker.hpp>

using namespace gulc;

// TODO: Our process for resolving types should be:
//       1. Resolve a type's inheritance, resolve any types needing for this and solve any constants needed
//       2. Now we have all type declarations resolved, go through functions and variables resolving their types
//       3. Done?

int main() {
    std::vector<std::string> filePaths {
            "examples/TestFile.gul"
    };
    std::vector<ASTFile> parsedFiles(filePaths.size());

    for (std::size_t i = 0; i < filePaths.size(); ++i) {
        Parser parser;
        parsedFiles.push_back(parser.parseFile(i, filePaths[i]));
    }

    // Resolve the bases of structs, classes, traits, enums, etc.
    BaseResolver baseResolver(filePaths);
    baseResolver.processFiles(parsedFiles);

    // Validate there are no circular references in any declarations
    CircularReferenceChecker circularReferenceChecker(filePaths);
    circularReferenceChecker.processFiles(parsedFiles);

    // Resolve all `const` declarations, types, etc. first
    ConstTypeResolver constTypeResolver(filePaths);
    constTypeResolver.processFiles(parsedFiles);

//    TypeResolver typeResolver(filePaths);
//    typeResolver.processFiles(parsedFiles);

    return 0;
}
