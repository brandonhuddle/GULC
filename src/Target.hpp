#ifndef GULC_TARGET_HPP
#define GULC_TARGET_HPP

#include <cstddef>

namespace gulc {
    class Target {
    public:
        // Currently only support x86_64
        enum class Arch {
            x86_64
        };

        enum class OS {
            Linux,
            // TODO: Fix the weird LLVM bug found on Windows caused by the LLVM Triple string destructor being weird,
            //       (bug seems to be out of our hands, seems to be a bug within the Windows C++ library?)
            Windows
        };

        enum class Env {
            GNU,
            MSVC
        };

    private:
        Arch _arch;
        OS _os;
        Env _env;

        std::size_t _sizeofPtr;
        /// sizeof(usize) == sizeof(isize)
        std::size_t _sizeofUSize;

        /// The align of for struct also tells us how to pad the struct
        std::size_t _alignofStruct;

    public:
        Target(Arch arch, OS os, Env env);

        Arch getArch() const { return _arch; }
        OS getOS() const { return _os; }
        Env getEnv() const { return _env; }

        std::size_t sizeofPtr() const { return _sizeofPtr; }
        std::size_t sizeofUSize() const { return _sizeofUSize; }
        std::size_t sizeofISize() const { return _sizeofUSize; }

        std::size_t alignofStruct() const { return _alignofStruct; }

        static Target getHostTarget();

    };
}

#endif //GULC_TARGET_HPP
