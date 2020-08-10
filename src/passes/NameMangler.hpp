#ifndef GULC_NAMEMANGLER_HPP
#define GULC_NAMEMANGLER_HPP

#include <namemangling/ManglerBase.hpp>
#include <parsing/ASTFile.hpp>

namespace gulc {
    class NameMangler {
    public:
        explicit NameMangler(ManglerBase* manglerBase)
                : _manglerBase(manglerBase) {}

        void processFiles(std::vector<ASTFile>& files);

    private:
        ManglerBase* _manglerBase;

        void processTypeDecl(Decl* decl);
        void processDecl(Decl* decl);

    };
}

#endif //GULC_NAMEMANGLER_HPP
