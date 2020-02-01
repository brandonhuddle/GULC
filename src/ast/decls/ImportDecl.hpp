#ifndef GULC_IMPORTDECL_HPP
#define GULC_IMPORTDECL_HPP

#include <ast/Decl.hpp>
#include <vector>
#include <optional>

namespace gulc {
    class ImportDecl : public Decl {
    public:
        static bool classof(const Decl* decl) { return decl->getDeclKind() == Decl::Kind::Import; }

        ImportDecl(unsigned int sourceFileID, std::vector<Attr*> attributes,
                   TextPosition importStartPosition, TextPosition importEndPosition,
                   std::vector<Identifier> importPath)
                : Decl(Decl::Kind::Import, sourceFileID, std::move(attributes),
                       Decl::Visibility::Unassigned, false, {}),
                  _importStartPosition(importStartPosition), _importEndPosition(importEndPosition),
                  _importPath(std::move(importPath)), _asStartPosition(), _asEndPosition(),
                  _importAlias() {}

        ImportDecl(unsigned int sourceFileID, std::vector<Attr*> attributes,
                   TextPosition importStartPosition, TextPosition importEndPosition,
                   std::vector<Identifier> importPath, TextPosition asStartPosition, TextPosition asEndPosition,
                   Identifier importAlias)
                : Decl(Decl::Kind::Import, sourceFileID, std::move(attributes),
                       Decl::Visibility::Unassigned, false, {}),
                  _importStartPosition(importStartPosition), _importEndPosition(importEndPosition),
                  _importPath(std::move(importPath)), _asStartPosition(asStartPosition), _asEndPosition(asEndPosition),
                  _importAlias(importAlias) {}

        TextPosition importStartPosition() const { return _importStartPosition; }
        TextPosition importEndPosition() const { return _importEndPosition; }
        std::vector<Identifier> const& importPath() const { return _importPath; }
        bool hasAlias() const { return _importAlias.has_value(); }
        TextPosition asStartPosition() const { return _asStartPosition; }
        TextPosition asEndPosition() const { return _asEndPosition; }
        Identifier const& importAlias() const { return _importAlias.value(); }

        TextPosition startPosition() const override { return _importStartPosition; }
        TextPosition endPosition() const override {
            if (_importAlias) {
                return _importAlias->endPosition();
            } else if (_importPath.empty()) {
                // If `importPath` is empty something went wrong but we won't error here. We will error out elsewhere
                // NOTE: This really won't ever happen. The parser will error out before creating this decl
                return _importEndPosition;
            } else {
                return _importPath.back().endPosition();
            }
        }

    protected:
        // These are the start and end positions just for the keyword `import`
        const TextPosition _importStartPosition;
        const TextPosition _importEndPosition;
        // E.g. `std.io`
        const std::vector<Identifier> _importPath;
        // These are the start and end positions just for the keyword `as`
        const TextPosition _asStartPosition;
        const TextPosition _asEndPosition;
        const std::optional<Identifier> _importAlias;

    };
}

#endif //GULC_IMPORTDECL_HPP
