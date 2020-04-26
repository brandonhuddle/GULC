#ifndef GULC_DECLMODIFIERS_HPP
#define GULC_DECLMODIFIERS_HPP

namespace gulc {
    enum class DeclModifiers {
        None = 0,
        Static = 1u << 0u,
        Mut = 1u << 1u,
        Volatile = 1u << 2u,
        Abstract = 1u << 3u,
        Virtual = 1u << 4u,
        Override = 1u << 5u,
        Extern = 1u << 6u,
        // Used for `func example();` with no body
        Prototype = 1u << 7u,
    };

    inline DeclModifiers operator|(DeclModifiers left, DeclModifiers right) { return static_cast<DeclModifiers>(static_cast<int>(left) | static_cast<int>(right)); }
    inline DeclModifiers operator&(DeclModifiers left, DeclModifiers right) { return static_cast<DeclModifiers>(static_cast<int>(left) & static_cast<int>(right)); }
    inline DeclModifiers operator^(DeclModifiers left, DeclModifiers right) { return static_cast<DeclModifiers>(static_cast<int>(left) ^ static_cast<int>(right)); }

    inline DeclModifiers& operator|=(DeclModifiers& left, DeclModifiers right) { left = left | right; return left; }
    inline DeclModifiers& operator&=(DeclModifiers& left, DeclModifiers right) { left = left & right; return left; }
    inline DeclModifiers& operator^=(DeclModifiers& left, DeclModifiers right) { left = left ^ right; return left; }
}

#endif //GULC_DECLMODIFIERS_HPP
