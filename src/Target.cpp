#include "Target.hpp"

using namespace gulc;

Target::Target(Target::Arch arch, Target::OS os, Target::Env env)
        : _arch(arch), _os(os), _env(env) {
    switch (arch) {
        case Arch::x86_64:
            _sizeofPtr = 8;
            _sizeofUSize = 8;

            _alignofStruct = 8;
            break;
    }
}

Target Target::getHostTarget() {
    // TODO: At some point we should actually implement detectors for this... but for now we just do this...
    return Target(Arch::x86_64, OS::Linux, Env::GNU);
}
