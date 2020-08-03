# INTRODUCTION
Ghoul is a modern, multi-paradigm programming language meant to offer the full capabilities of C++ in a syntax similar 
to Swift. Example syntax:
    
    import std.io;
    
    func main(_ args: []string) {
        // NOTE: `@for` is a custom macro that is defined to give Ghoul support for `foreach` loops. This is not a 
        //       built in feature. I've opted to make it somethat that comes from a user-definable macro to limit the 
        //       amount of built in features that will require compiler secrets to build (such as C++ compilers are 
        //       built to handle iterators as a special case. I dislike that and think macros should handle that 
        //       instead in a more generic approach)
        @for (index, arg) in args {
            // NOTE: Standard library not yet finalized, `Console` may never exist.
            Console.write("Index: {0}, Argument: {1}", index, arg)
        }
    }
    
Notable features:
 * Semicolons are optional, like Swift
 * Full C++-style template support, including `const` value templates (e.g. `struct array<const arrSize: usize> {}` 
   allows `let a: array<33>`)
 * Parenthesis aren't needed for control flow statements (e.g. `if true {}`, `while true {}`, 
   `for i = 0; i < arr.length; ++i {}`, etc.)
 * Immutable by default, mutability is explicit with `mut` keyword (e.g. `mut func example()` can modify `self`)
 * Noexcept by default, functions must be marked as `throws` to support `throw` 
   (e.g. `func example() throws std.io.errors.fileNotFound` the exact type isn't required but recommended)
 * Midori-inspired contracts supporting both `requres` and `ensures` 
   (e.g. `func example(_ param1: i32) requires param1 > 12 {}` calling `example(11)` will be caught and error by the 
   compiler)
 * Fully cross platform - I've spent nearly a decade of my life programming on all major operating systems and some 
   niche operating systems. The planned operating systems to support are Windows 10, macOS, iOS & family, Linux, and 
   Android. The long term goal is also to implement an operating system written entirely in Ghoul with a microkernel 
   but that is years away.
 * Fully extendable - Ghoul will support a complete, error checked macro system similar to Rust. Planned features to be 
   supported with it so far are `@for` to enable `foreach` loops from within the standard library and `@new` to enable 
   the ability to do `@new Window(title: "Hello, World!")` to create a `Window` object on the heap. Moving `new` to a 
   `@new` macro allows for multiple memory management styles to all have their own keyword and unties the compiler from 
   creating standard library specific code within the compiler itself.
 * `const` solving. `const` for Ghoul no longer means immutable like C++, `const` is closer related to C++'s 
   `constexpr` but more intense as anything within a `const` function MUST be solvable at compile time. `const` 
   functions can only use `const` types, can only call other `const` functions, and should allow for more intense 
   compile time optimization and analysis.

One thing to note is that while Ghoul will support contracts it is not a 100% "Code by Contract" language. Contracts 
should supplement the language and be used where they make sense but unit testing and other methods of testing should 
still be used. Contracts can be very useful for both error messages but also compiler optimizations, they can be used 
to communicate to the compiler what specific pieces of code will be doing and can even lead the compiler to completely 
remove sections of code based on what it knows about the code in very specific situations (at its most extreme).

## Longer Examples
NOTE: See `examples/` for a list of some more intense syntax examples I used to test the compiler with.
#### Operator Overloading
    
    namespace std.math {
        trait Numerical {
            operator infix +(right: Self) -> Self
            operator infix -(right: Self) -> Self
            operator infix *(right: Self) -> Self
            operator infix /(right: Self) -> Self
            operator infix %(right: Self) -> Self
            // NOTE: ^^ is used for exponents.
            operator infix ^^(right: Self) -> Self
        }
        
        struct i128: Numerical {
            public operator infix +(right: Self) -> Self {
                // You get the idea...
            }
        }
    }
    
#### Templates
    
    struct array<IndexType, const length: usize> {
        private var flatArray: [IndexType; usize]
        
        init() {
            self.flatArray = [ 0... ]
        }
        
        // NOTE: Attribute not finalized, adds ability to initialize struct with an array that is outside the parameter 
        //       list like: `example() [ 1, 2, 3, 4 ]`
        init(@arrayBuilder _ flatArray: [IndexType; usize]) {
            self.flatArray = flatArray
        }
        
        subscript(_ index: usize) -> IndexType
        requires index >= 0
        requires index < length {
            return flayArray[index]
        }
    }
    
    func main(_ args: []string) {
        let exampleArray = array<i32, 3>() [ 1, 2, 3 ]
        let exampleArray2 = array<f32, 2>([ 1, 2 ])
    }
    
#### Labeled Loops
    
    func main() {
        xLoop:
        for x = 0; x < 12; ++x {
            yLoop:
            for y = 0; y < 12; ++y {
                zLoop:
                for z = 0; z < 12; ++z {
                    if (x + y + z) == 44 {
                        break xLoop
                    }
                }
            }
        }
    }
    
#### Advanced Templates + Traits
Templates have a `where` contract that can be used to apply constraints on the template parameters. The plan is to use 
`trait` as the general control for templates. An decent example of this is below:
    
    // Very basic trait to make `expr + expr` supported for the type...
    trait Addable {
        operator infix +(right: Self) -> Self
    }
    
    // Obviously this would be applied to ALL built in types...
    // NOTE: `i32` has the `i32 + i32` operator already defined so there is no need to reimplement it. We can find that 
    //       declaration and assign it to the `Addable::operator infix +(i32)` implicitly.
    extension i32: Addable {}
    
    struct SummableSet<ItemType>
    where ItemType: Addable {
        // Now `SummableSet<ItemType>` can only be used where `ItemType` implements `Addable` in the context it is used.
    }
    
The idea for the language is to implement traits for just about any functionality such as that. Without a trait defined 
you can't do anything with a templated type besides pass it around. This allows us to validate template code from the 
compiler before you ever reach the `SummableSet<Window>` and have to deal with the mess that is C++'s template errors. 
In Ghoul we would see that `Window` does NOT implement `Addable` and error out saying such.
    
    struct ErrorExample<T> {
        func example(left: T, right: T) -> T {
            // This would error out saying `T` doesn't have an `operator infix -(T)`. The error would be triggered 
            // before any template instantiations could be made, preventing any confusing errors that would require you 
            // to know the underlying syntax for the template to be able to read properly.
            return left - right
        }
    }
    
## Compiler Notes
Currently the compiler is still quite buggy. It does NOT account for mutability yet, you can call `mut` functions on 
an `immut` variable.

## Goals
Ghoul is just a for fun project of mine. There may never be ABI stability or API stability. The goals of the project 
are also extreme, "purist" goals that will slow down development but are what make the project fun to me. Some of the 
current main goals are:

 * Bootstrap compiler into the Ghoul programming language itself (first with a LLVM backend but later our own Ghoul 
   backend)
 * Custom Ghoul operating system. Ghoul spawned from my interest in making microkernels and operating systems in 
   general. The goal is to create a microkernel with 100% Ghoul, no other languages supported. All code would be 
   written by me. Including the very exciting and fun (to me) project of implementing an entire GPU driver on my 
   own in Ghoul. I've read it will take a lifetime and I'm counting on that because I have 60+ years left on this earth 
   and I gotta spend the time doing something fun.
   GPU will most likely be something made by AMD or Intel since they are the only two semi-friendly to open source. 
   That may change in the future (Maybe we'll get a RISC-V style GPU? Doubtful... but maybe)
 * Cross platform. I've spent my life working on all major operating systems. Started with Linux, then Windows, then 
   iOS, Android, Mac OS X, etc. I want to support them all.
 * GUI focused. The current state of GUI frameworks disappoint me (especially how long it has taken for Linux to gain 
   emoji support in the main frameworks). I've had an idea for a GUI library for a while now that I will implement for 
   all supported platforms (excited to see what I can do on convergent GUIs)
 * 100% Ghoul. There may be some areas we tie back to C (such as IO on Windows, much easier to handle through C) but 
   I want as much as possible to be made within Ghoul. Even if some platforms have sections not in Ghoul there must 
   always be at least 1 backend that is 100% Ghoul to allow for cross compiling to the Ghoul OS. That means no harfbuzz 
   for the GUI library (again, it will take a while but this is what I enjoy doing. If it takes 20 years I'll have 20 
   years of fun)
 * Standard cryptography library. Ghoul isn't meant for production so I'll be rolling my own crypto just for fun. 
   Definitely 100% do not use it, there should be a version of it with the same API but using a battle tested crypto 
   backend that would be safer for production. Again, just for fun.
 * 100% Ghoul IDE - I've always enjoyed making text editors and syntax highlighting text editors. But I would love to 
   make an IDE in Ghoul for Ghoul at some point. Most likely we will just have a language server in the beginning as 
   that is the easiest to get Ghoul working in other IDEs. But making an IDE is more fun.

With the goals set out above I should be complete within maybe the next 100-200 years. But Ghoul the programming 
language, without the 100% Ghoul compiler and with a more basic standard library should be done within a year or two.
