#include <parsing/Parser.hpp>
#include <passes/BasicTypeResolver.hpp>
#include <passes/DeclInstantiator.hpp>
#include "Target.hpp"

using namespace gulc;


int main() {
    gulc::Target target = gulc::Target::getHostTarget();

    std::vector<std::string> filePaths {
            "examples/TestFile.gul"
    };
    std::vector<ASTFile> parsedFiles;

    for (std::size_t i = 0; i < filePaths.size(); ++i) {
        Parser parser;
        parsedFiles.push_back(parser.parseFile(i, filePaths[i]));
    }

    // TODO:
    //  1. Finish support for `ValueLiteralExpr` usage in templates
    //  2. Verify that our template system works properly in more niche scenarios
    //  3. DeclResolver - Resolve any Decl references within any Stmt or Expr
    //  4. ???
    //  5. Be poor because this is open source.

    // Resolve all types as much as possible, leaving `TemplatedType`s for any templates
    BasicTypeResolver basicTypeResolver(filePaths);
    basicTypeResolver.processFiles(parsedFiles);

    DeclInstantiator declInstantiator(target, filePaths);
    declInstantiator.processFiles(parsedFiles);

    return 0;
}
