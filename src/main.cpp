#include <parsing/Parser.hpp>
#include <passes/TypeResolver.hpp>
#include <passes/ConstTypeResolver.hpp>

using namespace gulc;

int main() {
    std::vector<std::string> filePaths {
            "examples/TestFile.gul"
    };
    std::vector<ASTFile> parsedFiles(filePaths.size());

    for (std::size_t i = 0; i < filePaths.size(); ++i) {
        Parser parser;
        parsedFiles.push_back(parser.parseFile(i, filePaths[i]));
    }

    // Resolve all `const` declarations, types, etc. first
    ConstTypeResolver constTypeResolver(filePaths);
    constTypeResolver.processFiles(parsedFiles);

//    TypeResolver typeResolver(filePaths);
//    typeResolver.processFiles(parsedFiles);

    return 0;
}
