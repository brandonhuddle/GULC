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
#ifndef GULC_EXPR_HPP
#define GULC_EXPR_HPP

#include "Stmt.hpp"
#include "Type.hpp"
#include <string>

namespace gulc {
    class __ExprStmtFix : public Stmt {
    public:
        explicit __ExprStmtFix(Stmt::Kind stmtKind)
                : __ExprStmtFix(Node::Kind::Stmt, stmtKind) {}
        __ExprStmtFix(Node::Kind nodeKind, Stmt::Kind stmtKind)
                : Stmt(nodeKind, stmtKind) {}

        Stmt* deepCopy() const override { return deepCopyStmt(); }

    protected:
        virtual Stmt* deepCopyStmt() const = 0;

    };

    class Expr : public __ExprStmtFix {
    public:
        static bool classof(const Node* node) { return node->getNodeKind() == Node::Kind::Expr; }
        static bool classof(const Stmt* stmt) { return stmt->getStmtKind() == Stmt::Kind::Expr; }

        enum class Kind {
            ArrayLiteral,
            As,
            AssignmentOperator,
            BoolLiteral,
            CallOperatorReference,
            CheckExtendsType,
            ConstructorCall,
            ConstructorReference,
            CurrentSelf,
            DestructorCall,
            DestructorReference,
            EnumConstRef,
            FunctionCall,
            FunctionReference,
            Has,
            Identifier,
            ImplicitCast,
            ImplicitDeref,
            InfixOperator,
            Is,
            LabeledArgument,
            LocalVariableRef,
            LValueToRValue,
            MemberAccessCall,
            MemberFunctionCall,
            MemberPostfixOperatorCall,
            MemberPrefixOperatorCall,
            MemberPropertyRef,
            MemberSubscriptCall,
            MemberVariableRef,
            ParameterRef,
            Paren,
            PostfixOperator,
            PrefixOperator,
            PropertyGetCall,
            PropertyRef,
            PropertySetCall,
            Ref,
            StructAssignmentOperator,
            SubscriptCall,
            SubscriptRef,
            TemplateConstRef,
            TemporaryValueRef,
            Ternary,
            Try,
            Type,
            ValueLiteral,
            VariableDecl,
            VariableRef,
            VTableFunctionReference,
        };

        Expr::Kind getExprKind() const { return _exprKind; }
        // I'm tired of seeing clang complain about this not being marked `override` in every file.
        // It isn't an override, it is SHADOWING. Annoying.
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Winconsistent-missing-override"
        virtual Expr* deepCopy() const = 0;
#pragma clang diagnostic pop
        virtual std::string toString() const = 0;

        // This is the type for the current value (i.e. `12 + 12` would have the type `i32`, function calls would be
        // the type of the function's return type)
        Type* valueType;

        ~Expr() override {
            delete valueType;
        }

    protected:
        Expr::Kind _exprKind;

        explicit Expr(Expr::Kind exprKind)
                : __ExprStmtFix(Node::Kind::Expr, Stmt::Kind::Expr),
                  valueType(nullptr), _exprKind(exprKind) {}

        Stmt* deepCopyStmt() const override {
            return deepCopy();
        }

    };
}

#endif //GULC_EXPR_HPP
