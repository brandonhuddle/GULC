cmake_minimum_required(VERSION 3.15)
project(gulc)

set(CMAKE_CXX_STANDARD 17)

if(APPLE)
    # NOTE: `brew install llvm@6`
    set(ENV{LLVM_DIR} "/usr/local/opt/llvm@6/lib/cmake")
endif()

find_package(LLVM REQUIRED CONFIG)
message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

include_directories(${LLVM_INCLUDE_DIRS})
if(LLVM_BUILD_MAIN_SRC_DIR)
    include_directories(${LLVM_BUILD_MAIN_SRC_DIR}/tools/clang/include)
    include_directories(${LLVM_BUILD_BINARY_DIR}/tools/clang/include)
endif()
link_directories(${LLVM_LIBRARY_DIRS})
add_definitions(${LLVM_DEFINITIONS})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/)

add_executable(gulc
        src/main.cpp
        src/make_reverse_iterator.cpp
        src/make_reverse_iterator.hpp
        src/Target.cpp
        src/Target.hpp

        src/ast/Node.cpp
        src/ast/Node.hpp

        src/ast/Identifier.cpp
        src/ast/Identifier.hpp

        src/ast/Attr.cpp
        src/ast/Attr.hpp
        src/ast/attrs/CopyAttr.cpp
        src/ast/attrs/CopyAttr.hpp
        src/ast/attrs/CustomAttr.cpp
        src/ast/attrs/CustomAttr.hpp
        src/ast/attrs/UnresolvedAttr.cpp
        src/ast/attrs/UnresolvedAttr.hpp

        src/ast/Decl.cpp
        src/ast/Decl.hpp
        src/ast/DeclModifiers.cpp
        src/ast/DeclModifiers.hpp
        src/ast/decls/CallOperatorDecl.cpp
        src/ast/decls/CallOperatorDecl.hpp
        src/ast/decls/ConstructorDecl.cpp
        src/ast/decls/ConstructorDecl.hpp
        src/ast/decls/DestructorDecl.cpp
        src/ast/decls/DestructorDecl.hpp
        src/ast/decls/EnumDecl.cpp
        src/ast/decls/EnumDecl.hpp
        src/ast/decls/EnumConstDecl.cpp
        src/ast/decls/EnumConstDecl.hpp
        src/ast/decls/ExtensionDecl.cpp
        src/ast/decls/ExtensionDecl.hpp
        src/ast/decls/FunctionDecl.cpp
        src/ast/decls/FunctionDecl.hpp
        src/ast/decls/ImportDecl.cpp
        src/ast/decls/ImportDecl.hpp
        src/ast/decls/NamespaceDecl.cpp
        src/ast/decls/NamespaceDecl.hpp
        src/ast/decls/OperatorDecl.cpp
        src/ast/decls/OperatorDecl.hpp
        src/ast/decls/ParameterDecl.cpp
        src/ast/decls/ParameterDecl.hpp
        src/ast/decls/PropertyDecl.cpp
        src/ast/decls/PropertyDecl.hpp
        src/ast/decls/PropertyGetDecl.cpp
        src/ast/decls/PropertyGetDecl.hpp
        src/ast/decls/PropertySetDecl.cpp
        src/ast/decls/PropertySetDecl.hpp
        src/ast/decls/StructDecl.cpp
        src/ast/decls/StructDecl.hpp
        src/ast/decls/SubscriptOperatorDecl.cpp
        src/ast/decls/SubscriptOperatorDecl.hpp
        src/ast/decls/SubscriptOperatorGetDecl.cpp
        src/ast/decls/SubscriptOperatorGetDecl.hpp
        src/ast/decls/SubscriptOperatorSetDecl.cpp
        src/ast/decls/SubscriptOperatorSetDecl.hpp
        src/ast/decls/TemplateFunctionDecl.cpp
        src/ast/decls/TemplateFunctionDecl.hpp
        src/ast/decls/TemplateFunctionInstDecl.cpp
        src/ast/decls/TemplateFunctionInstDecl.hpp
        src/ast/decls/TemplateParameterDecl.cpp
        src/ast/decls/TemplateParameterDecl.hpp
        src/ast/decls/TemplateStructDecl.cpp
        src/ast/decls/TemplateStructDecl.hpp
        src/ast/decls/TemplateStructInstDecl.cpp
        src/ast/decls/TemplateStructInstDecl.hpp
        src/ast/decls/TemplateTraitDecl.cpp
        src/ast/decls/TemplateTraitDecl.hpp
        src/ast/decls/TemplateTraitInstDecl.cpp
        src/ast/decls/TemplateTraitInstDecl.hpp
        src/ast/decls/TraitDecl.cpp
        src/ast/decls/TraitDecl.hpp
        src/ast/decls/TypeAliasDecl.cpp
        src/ast/decls/TypeAliasDecl.hpp
        src/ast/decls/TypeSuffixDecl.cpp
        src/ast/decls/TypeSuffixDecl.hpp
        src/ast/decls/VariableDecl.cpp
        src/ast/decls/VariableDecl.hpp

        src/ast/Type.cpp
        src/ast/Type.hpp
        src/ast/types/AliasType.cpp
        src/ast/types/AliasType.hpp
        src/ast/types/BuiltInType.cpp
        src/ast/types/BuiltInType.hpp
        src/ast/types/BoolType.cpp
        src/ast/types/BoolType.hpp
        src/ast/types/DependentType.cpp
        src/ast/types/DependentType.hpp
        src/ast/types/DimensionType.cpp
        src/ast/types/DimensionType.hpp
        src/ast/types/EnumType.cpp
        src/ast/types/EnumType.hpp
        src/ast/types/FlatArrayType.cpp
        src/ast/types/FlatArrayType.hpp
        src/ast/types/FunctionPointerType.cpp
        src/ast/types/FunctionPointerType.hpp
        src/ast/types/LabeledType.cpp
        src/ast/types/LabeledType.hpp
        src/ast/types/PointerType.cpp
        src/ast/types/PointerType.hpp
        src/ast/types/ReferenceType.cpp
        src/ast/types/ReferenceType.hpp
        src/ast/types/SelfType.cpp
        src/ast/types/SelfType.hpp
        src/ast/types/StructType.cpp
        src/ast/types/StructType.hpp
        src/ast/types/TemplatedType.cpp
        src/ast/types/TemplatedType.hpp
        src/ast/types/TemplateStructType.cpp
        src/ast/types/TemplateStructType.hpp
        src/ast/types/TemplateTraitType.cpp
        src/ast/types/TemplateTraitType.hpp
        src/ast/types/TemplateTypenameRefType.cpp
        src/ast/types/TemplateTypenameRefType.hpp
        src/ast/types/TraitType.cpp
        src/ast/types/TraitType.hpp
        src/ast/types/UnresolvedNestedType.cpp
        src/ast/types/UnresolvedNestedType.hpp
        src/ast/types/UnresolvedType.cpp
        src/ast/types/UnresolvedType.hpp
        src/ast/types/VTableType.cpp
        src/ast/types/VTableType.hpp

        src/ast/Expr.cpp
        src/ast/Expr.hpp
        src/ast/exprs/ArrayLiteralExpr.cpp
        src/ast/exprs/ArrayLiteralExpr.hpp
        src/ast/exprs/AsExpr.cpp
        src/ast/exprs/AsExpr.hpp
        src/ast/exprs/AssignmentOperatorExpr.cpp
        src/ast/exprs/AssignmentOperatorExpr.hpp
        src/ast/exprs/BoolLiteralExpr.cpp
        src/ast/exprs/BoolLiteralExpr.hpp
        src/ast/exprs/CallOperatorReferenceExpr.cpp
        src/ast/exprs/CallOperatorReferenceExpr.hpp
        src/ast/exprs/CheckExtendsTypeExpr.cpp
        src/ast/exprs/CheckExtendsTypeExpr.hpp
        src/ast/exprs/ConstructorCallExpr.cpp
        src/ast/exprs/ConstructorCallExpr.hpp
        src/ast/exprs/ConstructorReferenceExpr.cpp
        src/ast/exprs/ConstructorReferenceExpr.hpp
        src/ast/exprs/CurrentSelfExpr.cpp
        src/ast/exprs/CurrentSelfExpr.hpp
        src/ast/exprs/DestructorCallExpr.cpp
        src/ast/exprs/DestructorCallExpr.hpp
        src/ast/exprs/DestructorReferenceExpr.cpp
        src/ast/exprs/DestructorReferenceExpr.hpp
        src/ast/exprs/FunctionCallExpr.cpp
        src/ast/exprs/FunctionCallExpr.hpp
        src/ast/exprs/FunctionReferenceExpr.cpp
        src/ast/exprs/FunctionReferenceExpr.hpp
        src/ast/exprs/EnumConstRefExpr.cpp
        src/ast/exprs/EnumConstRefExpr.hpp
        src/ast/exprs/HasExpr.cpp
        src/ast/exprs/HasExpr.hpp
        src/ast/exprs/IdentifierExpr.cpp
        src/ast/exprs/IdentifierExpr.hpp
        src/ast/exprs/ImplicitCastExpr.cpp
        src/ast/exprs/ImplicitCastExpr.hpp
        src/ast/exprs/ImplicitDerefExpr.cpp
        src/ast/exprs/ImplicitDerefExpr.hpp
        src/ast/exprs/InfixOperatorExpr.cpp
        src/ast/exprs/InfixOperatorExpr.hpp
        src/ast/exprs/IsExpr.cpp
        src/ast/exprs/IsExpr.hpp
        src/ast/exprs/LabeledArgumentExpr.cpp
        src/ast/exprs/LabeledArgumentExpr.hpp
        src/ast/exprs/LocalVariableRefExpr.cpp
        src/ast/exprs/LocalVariableRefExpr.hpp
        src/ast/exprs/LValueToRValueExpr.cpp
        src/ast/exprs/LValueToRValueExpr.hpp
        src/ast/exprs/MemberAccessCallExpr.cpp
        src/ast/exprs/MemberAccessCallExpr.hpp
        src/ast/exprs/MemberFunctionCallExpr.cpp
        src/ast/exprs/MemberFunctionCallExpr.hpp
        src/ast/exprs/MemberPostfixOperatorCallExpr.cpp
        src/ast/exprs/MemberPostfixOperatorCallExpr.hpp
        src/ast/exprs/MemberPrefixOperatorCallExpr.cpp
        src/ast/exprs/MemberPrefixOperatorCallExpr.hpp
        src/ast/exprs/MemberPropertyRefExpr.cpp
        src/ast/exprs/MemberPropertyRefExpr.hpp
        src/ast/exprs/MemberSubscriptOperatorRefExpr.cpp
        src/ast/exprs/MemberSubscriptOperatorRefExpr.hpp
        src/ast/exprs/MemberVariableRefExpr.cpp
        src/ast/exprs/MemberVariableRefExpr.hpp
        src/ast/exprs/ParameterRefExpr.cpp
        src/ast/exprs/ParameterRefExpr.hpp
        src/ast/exprs/ParenExpr.cpp
        src/ast/exprs/ParenExpr.hpp
        src/ast/exprs/PostfixOperatorExpr.cpp
        src/ast/exprs/PostfixOperatorExpr.hpp
        src/ast/exprs/PrefixOperatorExpr.cpp
        src/ast/exprs/PrefixOperatorExpr.hpp
        src/ast/exprs/PropertyGetCallExpr.cpp
        src/ast/exprs/PropertyGetCallExpr.hpp
        src/ast/exprs/PropertyRefExpr.cpp
        src/ast/exprs/PropertyRefExpr.hpp
        src/ast/exprs/PropertySetCallExpr.cpp
        src/ast/exprs/PropertySetCallExpr.hpp
        src/ast/exprs/RefExpr.cpp
        src/ast/exprs/RefExpr.hpp
        src/ast/exprs/StructAssignmentOperatorExpr.cpp
        src/ast/exprs/StructAssignmentOperatorExpr.hpp
        src/ast/exprs/SubscriptCallExpr.cpp
        src/ast/exprs/SubscriptCallExpr.hpp
        src/ast/exprs/SubscriptOperatorGetCallExpr.cpp
        src/ast/exprs/SubscriptOperatorGetCallExpr.hpp
        src/ast/exprs/SubscriptOperatorRefExpr.cpp
        src/ast/exprs/SubscriptOperatorRefExpr.hpp
        src/ast/exprs/SubscriptOperatorSetCallExpr.cpp
        src/ast/exprs/SubscriptOperatorSetCallExpr.hpp
        src/ast/exprs/TemplateConstRefExpr.cpp
        src/ast/exprs/TemplateConstRefExpr.hpp
        src/ast/exprs/TemporaryValueRefExpr.cpp
        src/ast/exprs/TemporaryValueRefExpr.hpp
        src/ast/exprs/TernaryExpr.cpp
        src/ast/exprs/TernaryExpr.hpp
        src/ast/exprs/TryExpr.cpp
        src/ast/exprs/TryExpr.hpp
        src/ast/exprs/TypeExpr.cpp
        src/ast/exprs/TypeExpr.hpp
        src/ast/exprs/ValueLiteralExpr.cpp
        src/ast/exprs/ValueLiteralExpr.hpp
        src/ast/exprs/VariableDeclExpr.cpp
        src/ast/exprs/VariableDeclExpr.hpp
        src/ast/exprs/VariableRefExpr.cpp
        src/ast/exprs/VariableRefExpr.hpp
        src/ast/exprs/VTableFunctionReferenceExpr.cpp
        src/ast/exprs/VTableFunctionReferenceExpr.hpp

        src/ast/Stmt.cpp
        src/ast/Stmt.hpp
        src/ast/stmts/BreakStmt.cpp
        src/ast/stmts/BreakStmt.hpp
        src/ast/stmts/CaseStmt.cpp
        src/ast/stmts/CaseStmt.hpp
        src/ast/stmts/CatchStmt.cpp
        src/ast/stmts/CatchStmt.hpp
        src/ast/stmts/CompoundStmt.cpp
        src/ast/stmts/CompoundStmt.hpp
        src/ast/stmts/ContinueStmt.cpp
        src/ast/stmts/ContinueStmt.hpp
        src/ast/stmts/DoWhileStmt.cpp
        src/ast/stmts/DoWhileStmt.hpp
        src/ast/stmts/FallthroughStmt.cpp
        src/ast/stmts/FallthroughStmt.hpp
        src/ast/stmts/ForStmt.cpp
        src/ast/stmts/ForStmt.hpp
        src/ast/stmts/GotoStmt.cpp
        src/ast/stmts/GotoStmt.hpp
        src/ast/stmts/IfStmt.cpp
        src/ast/stmts/IfStmt.hpp
        src/ast/stmts/LabeledStmt.cpp
        src/ast/stmts/LabeledStmt.hpp
        src/ast/stmts/ReturnStmt.cpp
        src/ast/stmts/ReturnStmt.hpp
        src/ast/stmts/SwitchStmt.cpp
        src/ast/stmts/SwitchStmt.hpp
        src/ast/stmts/DoCatchStmt.cpp
        src/ast/stmts/DoCatchStmt.hpp
        src/ast/stmts/WhileStmt.cpp
        src/ast/stmts/WhileStmt.hpp

        src/ast/Cont.cpp
        src/ast/Cont.hpp
        src/ast/conts/RequiresCont.cpp
        src/ast/conts/RequiresCont.hpp
        src/ast/conts/EnsuresCont.cpp
        src/ast/conts/EnsuresCont.hpp
        src/ast/conts/ThrowsCont.cpp
        src/ast/conts/ThrowsCont.hpp
        src/ast/conts/WhereCont.cpp
        src/ast/conts/WhereCont.hpp

        src/parsing/ASTFile.cpp
        src/parsing/ASTFile.hpp
        src/parsing/Lexer.cpp
        src/parsing/Lexer.hpp
        src/parsing/TokenType.hpp
        src/parsing/Token.hpp
        src/parsing/Parser.cpp
        src/parsing/Parser.hpp

        src/passes/BasicDeclValidator.cpp
        src/passes/BasicDeclValidator.hpp
        src/passes/BasicTypeResolver.cpp
        src/passes/BasicTypeResolver.hpp
        src/passes/CodeProcessor.cpp
        src/passes/CodeProcessor.hpp
        src/passes/CodeTransformer.cpp
        src/passes/CodeTransformer.hpp
        src/passes/DeclInstantiator.cpp
        src/passes/DeclInstantiator.hpp
        src/passes/DeclInstValidator.cpp
        src/passes/DeclInstValidator.hpp
        src/passes/NameMangler.cpp
        src/passes/NameMangler.hpp
        src/passes/NamespacePrototyper.cpp
        src/passes/NamespacePrototyper.hpp

        src/utilities/ConstExprHelper.cpp
        src/utilities/ConstExprHelper.hpp
        src/utilities/ContractUtil.cpp
        src/utilities/ContractUtil.hpp
        src/utilities/FunctorUtil.cpp
        src/utilities/FunctorUtil.hpp
        src/utilities/InheritUtil.cpp
        src/utilities/InheritUtil.hpp
        src/utilities/SignatureComparer.cpp
        src/utilities/SignatureComparer.hpp
        src/utilities/SizeofUtil.cpp
        src/utilities/SizeofUtil.hpp
        src/utilities/TemplateCopyUtil.cpp
        src/utilities/TemplateCopyUtil.hpp
        src/utilities/TemplateInstHelper.cpp
        src/utilities/TemplateInstHelper.hpp
        src/utilities/TypeCompareUtil.cpp
        src/utilities/TypeCompareUtil.hpp
        src/utilities/TypeHelper.cpp
        src/utilities/TypeHelper.hpp

        src/namemangling/ManglerBase.cpp
        src/namemangling/ManglerBase.hpp
        src/namemangling/ItaniumMangler.cpp
        src/namemangling/ItaniumMangler.hpp

        src/linker/Linker.cpp
        src/linker/Linker.hpp

        src/codegen/CodeGen.cpp
        src/codegen/CodeGen.hpp
        src/codegen/Module.cpp
        src/codegen/Module.hpp

        src/objgen/ObjGen.cpp
        src/objgen/ObjGen.hpp
        src/objgen/ObjFile.cpp
        src/objgen/ObjFile.hpp)

# MSVC doesn't require the stdc++fs.
if (NOT MSVC AND NOT APPLE)
    target_link_libraries(gulc stdc++fs)
endif()

target_link_libraries(gulc
        LLVMCore
        LLVMTransformUtils
        LLVMScalarOpts
        LLVMInstCombine
        LLVMObject

        # Ugh there HAS to be an easier way to do this... TODO: Fix this. We don't need more half of these
        #        LLVMAArch64AsmParser
        #        LLVMAArch64AsmPrinter
        #        LLVMAArch64CodeGen
        #        LLVMAMDGPUAsmParser
        #        LLVMAMDGPUAsmPrinter
        #        LLVMAMDGPUCodeGen
        #        LLVMARMAsmParser
        #        LLVMARMAsmPrinter
        #        LLVMARMCodeGen
        #        LLVMBPFAsmParser
        #        LLVMBPFAsmPrinter
        #        LLVMBPFCodeGen
        #        LLVMHexagonAsmParser
        #        LLVMHexagonCodeGen
        #        LLVMLanaiAsmParser
        #        LLVMLanaiAsmPrinter
        #        LLVMLanaiCodeGen
        #        LLVMMipsAsmParser
        #        LLVMMipsAsmPrinter
        #        LLVMMipsCodeGen
        #        LLVMMSP430AsmParser
        #        LLVMMSP430AsmPrinter
        #        LLVMMSP430CodeGen
        #        LLVMNVPTXAsmPrinter
        #        LLVMNVPTXCodeGen
        #        LLVMPowerPCAsmParser
        #        LLVMPowerPCAsmPrinter
        #        LLVMPowerPCCodeGen
        #        LLVMSparcAsmParser
        #        LLVMSparcAsmPrinter
        #        LLVMSparcCodeGen
        #        LLVMSystemZAsmParser
        #        LLVMSystemZAsmPrinter
        #        LLVMSystemZCodeGen
        #        LLVMWebAssemblyAsmParser
        #        LLVMWebAssemblyAsmPrinter
        #        LLVMWebAssemblyCodeGen
        LLVMX86AsmParser
        LLVMX86AsmPrinter
        LLVMX86CodeGen
        #        LLVMXCoreAsmPrinter
        LLVMXCoreCodeGen
        LLVMPasses
)

# Copy some example files into the directory. This just makes it easier for me to test everything is working, this will be removed in the future.
add_custom_command(
        TARGET gulc POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/
        ${CMAKE_CURRENT_BINARY_DIR}/examples/)
