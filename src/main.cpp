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

// TODO: We need to clean up how we handle local variables. Anywhere where we access them we are manually regenerating
//       the list for the context. I think we could do better than that by storing a list for each context
// TODO: Templates need extended to support specialization. The `where` should NOT specialize.
//           struct partial_ref<T> { var m: ref T }
//           struct partial_ref<T: i32> { var m: T }
//       The idea is mainly just to give a way to specialize a template for certain types that might be better off
//       treated differently.

// TODO: Finish `ref` support
// TODO: Support `<=>`
// TODO: Support `func` in `enum` declarations
// TODO: Finish `prop` and `subscript` (they currently will crash the compiler)
// TODO: Finish `import` support (requires `implicitExtern` stuff)
// TODO: Finish operator overloading
// TODO: Finish extensions
// TODO: Finish templates (e.g. specialization and general usage)
// TODO: Support `trait`

// TODO: Improve error messages (e.g. add checks to validate everything is how it is supposed to be, output source code
//       of where the error went wrong, etc.)

// TODO: I think we should support `operator infix .()` or `operator prefix deref` to allow smart references.
//       C++ is starting to move towards the idea of `operator .()` and I really like the idea. I think this would be
//       nicer though:
//           struct box<T> {
//               private var ptr: *T
//  .
//               pure operator prefix deref() -> &T
//               requires ptr != null {
//                   return *T
//               }
//           }
//       The above would allow us to do:
//           let v: box<Window> = @new Window(title: "Hello, World!")
//  .
//           v.show()
//       While typing `v->show()` wouldn't be that big of a deal I really just don't want to require that. `box<T>`
//       is just a smart reference to a static reference, we shouldn't need to require `->`.
//       I think we should do `operator prefix deref()` and `operator prefix *()`
//       instead of `operator infix .()` and `operator infix ->()` to allow for us to us to call ALL operators on the
//       underlying reference type in a more natural/intuitive way. `operator infix .()` doesn't make me think that
//       `box<i32> + box<i32>` will call `operator infix +(_ rightValue: i32)`. BUT `operator prefix deref` does since
//       you will have to `deref` for `operator infix +(_ rightValue: i32)`.
//       To support this I think the general path for working with smart references is as follows:
//           if lValue { convertLValueToRValue(...) }
//           if smartRef { dereferenceSmartReferences(...) }
//           if ref { dereferenceReferences(...) }
//       Detection will obviously need to be a little more in depth but the above should work how it is needed. Unlike
//       normal references, we would double dereference. Smart references would be a layer above references.
//       I.e.:
//           let dRef: ref ref i32 = ...
//           let error: i32 = dRef // ERROR: `dRef` can only directly become `ref i32`, we won't double dereference
//           // BUT
//           let sRef: Ref<i32> = ...
//           let ok: i32 = sRef // OK: `Ref<i32>::operator prefix deref() -> ref i32` will automagically also dereference
//                              //     the result of the first `deref`. We only do that because returning `ref` is more natural.
//       NOTE: Overloading `operator prefix deref()` will make it so no other operators besides the following are allowed:
//           * `operator as<T>()`
//           * `operator is<T>()`
//       Any other operator would be pass to `T` instead. The reason for `as` and `is` being supported is to allow the
//       smart reference to implement basic support for runtime type reflection etc.
//       How would you support functions on a smart reference? Easy. You don't. A smart reference can't have functions.
//       If you DO add functions the only way to access them would be through full path signatures:
//           struct box<T> {
//               func getPointer() -> *T { ... }
//           }
//           let window: box<Window> = ...
//           let windowPtr: *Window = box<Window>::getPointer(window)

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
