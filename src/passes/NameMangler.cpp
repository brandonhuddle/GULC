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
#include "NameMangler.hpp"

void gulc::NameMangler::processFiles(std::vector<ASTFile>& files) {
    // Prepass to mangle declared type names
    for (ASTFile& fileAst : files) {
        for (Decl *decl : fileAst.declarations) {
            processTypeDecl(decl);
        }
    }

    for (ASTFile& fileAst : files) {
        for (Decl *decl : fileAst.declarations) {
            processDecl(decl);
        }
    }
}

void gulc::NameMangler::processTypeDecl(gulc::Decl* decl) {
    switch (decl->getDeclKind()) {
        case Decl::Kind::Enum:
            _manglerBase->mangleDecl(llvm::dyn_cast<EnumDecl>(decl));
            break;
        case Decl::Kind::Namespace:
            _manglerBase->mangleDecl(llvm::dyn_cast<NamespaceDecl>(decl));
            break;
        case Decl::Kind::TemplateStructInst:
        case Decl::Kind::Struct:
            _manglerBase->mangleDecl(llvm::dyn_cast<StructDecl>(decl));
            break;
        case Decl::Kind::TemplateStruct:
            _manglerBase->mangleDecl(llvm::dyn_cast<TemplateStructDecl>(decl));
            break;
        case Decl::Kind::TemplateTraitInst:
        case Decl::Kind::Trait:
            _manglerBase->mangleDecl(llvm::dyn_cast<TraitDecl>(decl));
            break;
        case Decl::Kind::TemplateTrait:
            _manglerBase->mangleDecl(llvm::dyn_cast<TemplateTraitDecl>(decl));
            break;
    }
}

void gulc::NameMangler::processDecl(gulc::Decl* decl) {
    switch (decl->getDeclKind()) {
        case Decl::Kind::Function:
            _manglerBase->mangle(llvm::dyn_cast<FunctionDecl>(decl));
            break;
        case Decl::Kind::TemplateFunction:
            _manglerBase->mangle(llvm::dyn_cast<TemplateFunctionDecl>(decl));
            break;
        case Decl::Kind::Operator:
            _manglerBase->mangle(llvm::dyn_cast<OperatorDecl>(decl));
            break;
        case Decl::Kind::CallOperator:
            _manglerBase->mangle(llvm::dyn_cast<CallOperatorDecl>(decl));
            break;
        case Decl::Kind::Variable:
            _manglerBase->mangle(llvm::dyn_cast<VariableDecl>(decl));
            break;
        case Decl::Kind::Namespace:
            _manglerBase->mangle(llvm::dyn_cast<NamespaceDecl>(decl));
            break;
        case Decl::Kind::TemplateStructInst:
        case Decl::Kind::Struct:
            _manglerBase->mangle(llvm::dyn_cast<StructDecl>(decl));
            break;
        case Decl::Kind::TemplateStruct:
            _manglerBase->mangle(llvm::dyn_cast<TemplateStructDecl>(decl));
            break;
        case Decl::Kind::TemplateTraitInst:
        case Decl::Kind::Trait:
            _manglerBase->mangle(llvm::dyn_cast<TraitDecl>(decl));
            break;
        case Decl::Kind::TemplateTrait:
            _manglerBase->mangle(llvm::dyn_cast<TemplateTraitDecl>(decl));
            break;
    }
}
