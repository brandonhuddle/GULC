## Basic Syntax
The basic syntax of Ghoul is very similar to Swift with a little bit of C++, Rust, and C# mixed in. The basic semantics 
of the language is also inspired by the above languages and Midori (as evident in Ghoul's use of contracts and 
`try $expr`).

NOTE: For ALL syntax documentation we are NOT showing off the standard library or any standard functions. Standard 
library work has not begun and as such any references to `std` or any functions are not considered final or even 
considered an example for how the final standard will work.

#### Basic Program
    
    import std.io
    import std.io.console
    
    func main(args: []string) -> i32 {
        @for arg in args {
            print("Argument: \"{0}\"", arg)
        }
        
        return 0
    }
    
Note above that there are no semicolons, semicolons are entirely optional in Ghoul and work very similar to Swift.
The `@for` is the current WIP syntax for a macro, the foreach style loop is implemented as a macro to separate our 
iterators from the compiler implementation in a way that is generic enough to allow entirely new syntax to be 
implemented in a library.

#### Variables
Variables are most similar to Rust with a few minor differences. Please note that, like Rust, Ghoul is immutable by 
default.
    
    func main() {
        let x: i32 = 0
        // Variable type can also be inferred
        let y = 12 // `y` is `i32`
        
        // Uncomment for error, `x` is `immut`
        //x = 3
        
        // `z` is a mutable single precision float
        let mut z: f32 = 1.0f32
        // This is OK
        z = 3.0f32
    }
    

#### Functions
Function syntax is nearly identical to Swift with a few small tweaks and additions we will discuss elsewhere. The basic 
syntax for a function is:
    
    func funcName(argLabel paramLabel: type) -> returnType {
        return val
    }
    
To call the above function you would do `funcName(argLabel: val)`. Same as Swift, to ignore the `argLabel` you do:
    
    func funcName(_ paramLabel: type) -> returnType {
        return val
    }
    
Then to call the function you would do `funcName(val)`

Ghoul also fully supports C++ style templates. To declare a template function you do:
    
    func funcName<TemplateType, const constParam: usize>(_ param: TemplateType) -> TemplateType {
        return param
    }
    
Though, unlike C++ templated types can only be used with how they are "described" or "constrained" by contracts. 
We will discuss that idea more in-depth in the `Contracts` documentation.

#### Namespaces
Like C# and C++, Ghoul uses the namespace convention instead of modules for organization. Namespaces are much closer to 
C# than C++ though as we use `.` instead of `::` for their accessor:
    
    namespace example.nested {
        // NOTE: `const` differs from all other languages AFAIK (maybe closest to Rust?)
        //       `const` is a much stricter version of C++'s `constexpr` in that it MUST be compile time solveable
        const var constVariable: i32 = 12
        // NOTE: `static` works the same as C++ here BUT you CANNOT call any function marked as `throws` here. For 
        //       `throws` functions you will have to manually handle setting the initial value from a function.
        static var staticVariable: type = type()
        
        func basicFunction() {
            return
        }
    }
    
    func main() {
        // Access a declaration within a namespace with `.`
        example.nested.basicFunction()
    }
    
#### Import
In Ghoul we use `import` to import an entire namespace, similar to C#'s `using`
    
    import std.io
    // You can also `alias` namespaces using the `import ... as ...`
    import std.io.console as Console
    
    func main() {
        Console.print("Hello, World!")
        return
    }
    
