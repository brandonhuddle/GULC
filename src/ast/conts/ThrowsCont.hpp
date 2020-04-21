#ifndef GULC_THROWSCONT_HPP
#define GULC_THROWSCONT_HPP

#include <ast/Cont.hpp>
#include <optional>
#include <ast/Identifier.hpp>

namespace gulc {
    class ThrowsCont : public Cont {
    public:
        static bool classof(const Cont* cont) { return cont->getContKind() == Cont::Kind::Throws; }

        ThrowsCont(TextPosition startPosition, TextPosition endPosition)
                : Cont(Cont::Kind::Throws, startPosition, endPosition) {}
        ThrowsCont(TextPosition startPosition, TextPosition endPosition, Identifier exceptionType)
                : Cont(Cont::Kind::Throws, startPosition, endPosition),
                  _exceptionType(exceptionType) {}

        bool hasExceptionType() const { return _exceptionType.has_value(); }
        Identifier const& exceptionTypeIdentifier() const { return _exceptionType.value(); }

        Cont* deepCopy() const override {
            if (_exceptionType.has_value()) {
                return new ThrowsCont(_startPosition, _endPosition, _exceptionType.value());
            } else {
                return new ThrowsCont(_startPosition, _endPosition);
            }
        }

    protected:
        std::optional<Identifier> _exceptionType;

    };
}

#endif //GULC_THROWSCONT_HPP
