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
#include <parsing/Parser.hpp>
#include <passes/BasicTypeResolver.hpp>
#include <passes/DeclInstantiator.hpp>
#include <passes/NamespacePrototyper.hpp>
#include <passes/BasicDeclValidator.hpp>
#include <passes/CodeProcessor.hpp>
#include <passes/NameMangler.hpp>
#include <namemangling/ItaniumMangler.hpp>
#include <codegen/CodeGen.hpp>
#include <passes/CodeTransformer.hpp>
#include <objgen/ObjGen.hpp>
#include <linker/Linker.hpp>
#include "Target.hpp"

using namespace gulc;

// TODO: We need an alternative to `isize` and `usize`. While they are good for pointers they are NOT good for IO.
//       With IO we might be working with files larger than can be indexed with `isize` and `usize`. Need to account for
//       this with maybe an `fsize`, `osize`, `ioffset`, `uoffset`, or something else. Or maybe just make sure to use
//       `u64` in these scenarios.

// TODO: Next features to work on:
//       * [POSTPONED] Finish adding support for extensions...
//       * Improve error checking for Decls post-instantiation
//       * Improve `const` solving (and simplify expressions as much as possible in trivial situations like `i + 2 + 2` == `i + 4`)
//       * `peekStartPosition` needs to handle removing any whitespace. Currently doing `peekStartPosition` for statements within a function will return `0, 0` for properly formatted code.

// TODO: Sidenotes:
//       * `ghoul` should handle creating new projects for us (i.e. `ghoul new Example`, `ghoul new --template=[URI] Example`)
//       * `ghoul` should handle the build files (e.g. replacement for `cmake`, should be able to also handle calling
//         the C compiler and other language compilers)
//       * Basically `ghoul` should be a compiler, a build system, and a project creation tool

// TODO: We need to disable implicit string literal concatenation for `[ ... ]` array literals:
//       [
//           "This "
//           "should error on the above line saying it expected `,`"
//       ]
//       This should be allowed everywhere else EXCEPT in an array literal. If you use parenthesis the error goes away
//       [
//           ("This "
//           "is allowed sense it won't allow accidental concatenation")
//       ]
//       The reason we need to do this is because with us no longer needing `;` at the end of every line someone might
//       THINK we allow implicit `,` in this scenario. This also wouldn't error out so they would THINK everything
//       worked until runtime when they realize the array is now:
//       [ "This should error on the above line saying it expected `,`" ]
//       If we can avoid runtime errors I would like to as much as possible. So within `[ ... ]` array literals
//       disallow implicit string concatenation UNLESS the strings are contained within `(...)`.

// TODO: Should we consider changing attributes to an `@...` system instead of `[...]` like Swift?
//       Pros:
//           * Less confusing syntax for parameters
//             `func test([Escaping] _ callback: func())`
//             vs
//             `func test(_ callback: @escaping func())`
//             We couldn't do `func test(_ callback: [Escaping] func())` as it would be confused for array syntax
//           * Could be extended to allow fully custom syntax
//             i.e. `@xmlClass Example {}` for an XML serializable class (AWFUL practice, don't do it. But basic
//             example.)
//           * Could potential be used to replace the rust macros `foreach!` stuff to allow:
//             @for i in 0..<12 {}
//           * More common. Swift, Kotlin, Java, etc. all use `@` nearly exactly the same way. C# uses `[...]`, Rust
//             uses `#[]`, C++ uses `[[...]]`, etc. It would be more uniform and well known to use `@...`
//       Cons:
//           * Uglier. Coming from C# I like `[XmlRoot] class AmazonEnvelope { ... }` better but I could grow to like
//             `@xmlRoot class AmazonEnvelope { ... }`
//           * That's really it. I just don't like how it looks. But having macros use `@...` as well would be nice I
//             guess. I do like `@for <item> in <iterator/enumerable> { ... }`

// TODO: For templates I think we should stop modifying them after `DeclInstantiator` and instead create a fake
//       instantiation with special parameters to handle validating the template. This would help with:
//        1. Further processing of a template will make creating new instantiations difficult to impossible
//        2. There are some aspects of templated types that are impossible process further without instantiating
//        3. For the validation instantiation we can create a `ConceptType` that is used to validate everything with a
//           template works.

// TODO: I think we should use the terms `abandon` and `abandonment` instead of `panic` even though Rust currently uses
//       `panic`.
//       Pros:
//        * Most descriptive to what is happening. `panic` doesn't really tell you what is actually happening and could
//          give the wrong impression that the program could keep going. Saying the process `abandoned` sounds more
//          descriptive and doesn't leave any impression that it could keep going or be fixed.
//        * More user friendly. When a process "abandons" for a user-facing program the most likely thing to happen
//          will be for the program to tell the user the program abandoned and how to report that to the developer.
//          In my opinion `abandon` will give the user a better idea on how severe this is than `panic`. Telling the
//          user the program `panicked` seems more confusing.
//       Cons:
//        * Rust already has set the idea up as `panic` (even though earlier implementations called in `abandon` as
//          well, such as the Midori OS's programming language)

// TODO: When within a constructor we need to ALWAYS use the current type being constructed's `vtable`. NOT the `vtable`
//       stored in memory. The reason for this is as follows:
//           class BaseClass {
//               var baseValue: i32
//  .
//               virtual func example() {
//  .                baseValue = 4
//               }
//  .
//               init() {
//                   baseValue = 12
//                   example()
//               }
//           }
//  .
//           class ChildClass: BaseClass {
//               var childValue: i32
//  .
//               override func example() {
//                   baseValue = childValue
//               }
//  .
//               init() {
//                   childValue = 45
//               }
//           }
//       In the above example `ChildClass::init()` will implicitly call `BaseClass::init()`. If we allow `init` to use
//       the `vtable` then `BaseClass::init::example()` will call `ChildClass::example()` which will assign `baseValue`
//       garbage data since `childValue` hasn't been initialized by that point.
//       To implement this my thoughts are that `init` should be a special case that completely ignores the `vtable` in
//       its entirety. `init` should call the exact functions it references, no vtable involved. That should solve this
//       issue.

// TODO: We should make a `@copy` attribute that tells the compiler the type should be copied by default instead of
//       moved by default.
//       Example:
//           @copy
//           struct m128 {
//               var a: f32
//               var b: f32
//               var c: f32
//               var d: f32
//           }
//       The above is an x86/x64 SIMD type. Since it would be considered a number it shouldn't be moved by default but
//       copied by default. Or maybe a better way to describe it is that it is a "value-type" instead of a
//       "container-type". I would need to research it a little more but in my mind "data containers" should be moved
//       while "value types" should be copied. We wouldn't be able to detect which is which on the compiler end I don't
//       think

// TODO: We need to clean up how we handle local variables. Anywhere where we access them we are manually regenerating
//       the list for the context. I think we could do better than that by storing a list for each context
// TODO: `ForStmt` currently has potential for a memory leak. We need to handle the `temporaryValues` for
//       `condition` and `iteration`

int main() {
    Target target = Target::getHostTarget();

    std::vector<std::string> filePaths {
            "examples/TestFile.ghoul"
//            "examples/TemplateWhereContractTest.ghoul"
    };
    std::vector<ASTFile> parsedFiles;

    for (std::size_t i = 0; i < filePaths.size(); ++i) {
        Parser parser;
        parsedFiles.push_back(parser.parseFile(i, filePaths[i]));
    }

    // Generate namespace map
    NamespacePrototyper namespacePrototyper;
    std::vector<NamespaceDecl*> prototypes = namespacePrototyper.generatePrototypes(parsedFiles);

    // Validate imports, check for obvious redefinitions, set the `Decl::container` member, etc.
    // TODO: Should we rename this to `BasicValidator` and move the label validation to here?
    //       I would go ahead and do it but I'm not sure if the overhead is worth it, I think it would be better
    //       to wait until we're doing more processing of the `Stmt`s...
    BasicDeclValidator basicDeclValidator(filePaths, prototypes);
    basicDeclValidator.processFiles(parsedFiles);

    // Resolve all types as much as possible, leaving `TemplatedType`s for any templates
    BasicTypeResolver basicTypeResolver(filePaths, prototypes);
    basicTypeResolver.processFiles(parsedFiles);

    // Instantiate Decl instances as much as possible (set `StructDecl` data layouts, instantiate `TemplatedType`, etc.)
    DeclInstantiator declInstantiator(target, filePaths);
    declInstantiator.processFiles(parsedFiles);

    // TODO: We need to actually implement `DeclInstValidator`
    //        * Check to make sure all `Self` type references are removed and are valid
    //        *

    // Process main code before IR generation
    CodeProcessor codeProcessor(target, filePaths, prototypes);
    codeProcessor.processFiles(parsedFiles);

    // Mangle decl names for code generation
    auto manglerBackend = ItaniumMangler();
    NameMangler nameMangler(&manglerBackend);
    nameMangler.processFiles(parsedFiles);

    // TODO: I think we could parallelize this and `CodeGen` since they don't modify any `Decl`
    CodeTransformer codeTransformer(target, filePaths, prototypes);
    codeTransformer.processFiles(parsedFiles);

    std::vector<ObjFile> objFiles;
    objFiles.reserve(parsedFiles.size());

    ObjGen::init();
    ObjGen objGen = ObjGen();

    for (auto parsedFile : parsedFiles) {
        // Generate LLVM IR
        CodeGen codeGen(target, filePaths);
        gulc::Module module = codeGen.generate(&parsedFile);

        // Generate the object files
        objFiles.push_back(objGen.generate(module));
    }

    gulc::Linker::link(objFiles);


    return 0;
}
