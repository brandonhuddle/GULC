/*
 * Copyright (C) 2020 Brandon Huddle
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
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
