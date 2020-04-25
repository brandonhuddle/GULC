cmake_minimum_required(VERSION 3.15)
project(gulc)

set(CMAKE_CXX_STANDARD 17)

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
        src/ast/decls/TemplateParameterDecl.cpp
        src/ast/decls/TemplateParameterDecl.hpp
        src/ast/decls/TemplateStructDecl.cpp
        src/ast/decls/TemplateStructDecl.hpp
        src/ast/decls/TemplateStructInstDecl.cpp
        src/ast/decls/TemplateStructInstDecl.hpp
        src/ast/decls/VariableDecl.cpp
        src/ast/decls/VariableDecl.hpp

        src/ast/Type.cpp
        src/ast/Type.hpp
        src/ast/types/BuiltInType.cpp
        src/ast/types/BuiltInType.hpp
        src/ast/types/DimensionType.cpp
        src/ast/types/DimensionType.hpp
        src/ast/types/FlatArrayType.cpp
        src/ast/types/FlatArrayType.hpp
        src/ast/types/PointerType.cpp
        src/ast/types/PointerType.hpp
        src/ast/types/ReferenceType.cpp
        src/ast/types/ReferenceType.hpp
        src/ast/types/StructType.cpp
        src/ast/types/StructType.hpp
        src/ast/types/TemplatedType.cpp
        src/ast/types/TemplatedType.hpp
        src/ast/types/TemplateTypenameRefType.cpp
        src/ast/types/TemplateTypenameRefType.hpp
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
        src/ast/exprs/FunctionCallExpr.cpp
        src/ast/exprs/FunctionCallExpr.hpp
        src/ast/exprs/HasExpr.cpp
        src/ast/exprs/HasExpr.hpp
        src/ast/exprs/IdentifierExpr.cpp
        src/ast/exprs/IdentifierExpr.hpp
        src/ast/exprs/IndexerCallExpr.cpp
        src/ast/exprs/IndexerCallExpr.hpp
        src/ast/exprs/InfixOperatorExpr.cpp
        src/ast/exprs/InfixOperatorExpr.hpp
        src/ast/exprs/IsExpr.cpp
        src/ast/exprs/IsExpr.hpp
        src/ast/exprs/LabeledArgumentExpr.cpp
        src/ast/exprs/LabeledArgumentExpr.hpp
        src/ast/exprs/MemberAccessCallExpr.cpp
        src/ast/exprs/MemberAccessCallExpr.hpp
        src/ast/exprs/ParenExpr.cpp
        src/ast/exprs/ParenExpr.hpp
        src/ast/exprs/PostfixOperatorExpr.cpp
        src/ast/exprs/PostfixOperatorExpr.hpp
        src/ast/exprs/PrefixOperatorExpr.cpp
        src/ast/exprs/PrefixOperatorExpr.hpp
        src/ast/exprs/TernaryExpr.cpp
        src/ast/exprs/TernaryExpr.hpp
        src/ast/exprs/TypeExpr.cpp
        src/ast/exprs/TypeExpr.hpp
        src/ast/exprs/ValueLiteralExpr.cpp
        src/ast/exprs/ValueLiteralExpr.hpp
        src/ast/exprs/VariableDeclExpr.cpp
        src/ast/exprs/VariableDeclExpr.hpp

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
        src/ast/stmts/DoStmt.cpp
        src/ast/stmts/DoStmt.hpp
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
        src/ast/stmts/TryStmt.cpp
        src/ast/stmts/TryStmt.hpp
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

        src/passes/BasicTypeResolver.cpp
        src/passes/BasicTypeResolver.hpp
        src/passes/DeclInstantiator.cpp
        src/passes/DeclInstantiator.hpp

        src/utilities/ConstExprHelper.cpp
        src/utilities/ConstExprHelper.hpp
        src/utilities/SignatureComparer.cpp
        src/utilities/SignatureComparer.hpp
        src/utilities/SizeofUtil.cpp
        src/utilities/SizeofUtil.hpp
        src/utilities/TemplateInstHelper.cpp
        src/utilities/TemplateInstHelper.hpp
        src/utilities/TypeHelper.cpp
        src/utilities/TypeHelper.hpp)

# MSVC doesn't require the stdc++fs.
if (NOT MSVC)
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
        #        LLVMXCoreCodeGen
)

# Copy some example files into the directory. This just makes it easier for me to test everything is working, this will be removed in the future.
add_custom_command(
        TARGET gulc POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        ${CMAKE_CURRENT_SOURCE_DIR}/examples/
        ${CMAKE_CURRENT_BINARY_DIR}/examples/)
