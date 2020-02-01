#ifndef GULC_ASTFILE_HPP
#define GULC_ASTFILE_HPP

#include <vector>
#include <ast/Decl.hpp>

namespace gulc {
    class ASTFile {
    public:
        unsigned int sourceFileID;
        std::vector<Decl*> declarations;

        ASTFile()
                : sourceFileID(0) {}
        ASTFile(unsigned int sourceFileID, std::vector<Decl*> declarations)
                : sourceFileID(sourceFileID), declarations(std::move(declarations)) {}

    };
}

#endif //GULC_ASTFILE_HPP
