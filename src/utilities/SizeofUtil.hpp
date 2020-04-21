#ifndef GULC_SIZEOFUTIL_HPP
#define GULC_SIZEOFUTIL_HPP

#include <cstddef>
#include <Target.hpp>
#include <ast/Type.hpp>

namespace gulc {
    struct SizeAndAlignment {
        std::size_t size;
        std::size_t align;

        SizeAndAlignment(std::size_t size, std::size_t align) : size(size), align(align) {}

    };

    class SizeofUtil {
    public:
        static SizeAndAlignment getSizeAndAlignmentOf(Target const& target, Type* type);

    };
}

#endif //GULC_SIZEOFUTIL_HPP
