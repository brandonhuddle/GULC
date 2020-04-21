#ifndef GULC_CUSTOMATTR_HPP
#define GULC_CUSTOMATTR_HPP

#include <ast/Attr.hpp>

namespace gulc {
    class CustomAttr : public Attr {
    public:
        static bool classof(const Attr* attr) { return attr->getAttrKind() == Attr::Kind::Custom; }

        // TODO: Requires `AttributeDecl`

        Attr* deepCopy() const override {

        }

    };
}

#endif //GULC_CUSTOMATTR_HPP
