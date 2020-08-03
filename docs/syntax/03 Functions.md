## Functions
Functions are similar to Swift with minor changes and the addition to contracts which I will give basic examples of 
here. For a more in-depth description of contracts, why we have them, and how to use them go to `Contracts`.
    
    func example(argLabel paramLabel: Type) -> Type {
        return paramLabel
    }
    
In the most basic example `argLabel` is the argument label that is required to be used when calling the function, 
`paramLabel` is the parameter name for the function scope, `Type` is the parameter type and the return type.

`argLabel` isn't required as shown:
    
    // Callable as `example(value)`
    func example(_ paramLabel: Type) -> Type {
        return paramLabel
    }
    
    // Callable as `example(param: value)`
    func example(param: Type) -> Type {
        return param
    }
    
Ghoul also has function overloading. You can overload on both the argument label and type for the function:
    
    func example(arg param: i32) -> i32 {
        return param
    }
    
    func example(arg param: f32) -> f32 {
        return param
    }
    
    func example(_ param: i32) -> i32 {
        return param
    }
    
    func example(_ param: f32) -> f32 {
        return param
    }
    
All above functions are valid and all can be differentiated between:
    
    example(arg: 12i32)
    example(arg: 12f32)
    example(4i32)
    example(4f32)
    
The example above would call each of the individual functions.

#### Exceptions
Unlike C++ and like Swift, Ghoul functions are `nothrow`/`noexcept` by default. To support throwing exceptions you must 
mark the function as `throws`:
    
    // Note how `throws` comes AFTER return type, there is a reason for that.
    func example() -> i32 throws {
        return 0
    }
    
The above tells all functions that want to call `example` that it can throw an exception and that needs to be accounted 
for.

Ghoul also allows specifying the exception types that will be thrown:
    
    func example() -> i32
    throws FileNotFoundException
    throws IOException {
        // ...
    }
    
This is not required but is highly recommended for better documentation. This also will make it so you CANNOT throw any 
exception except for `FileNotFoundException` and `IOException`. Adding an additional, untyped `throws` will allow any 
other type while still saying the important exceptions that will be thrown.

When accessing a function's pointer you should also note if the function `throws` the pointer must always say so:
    
    func example() throws {
        throw IOException()
    }
    
    func examle2() throws FileNotFoundException {
        throw FileNotFoundException()
    }
    
    // The below is illegal, it doesn't specify `throws` in the type signature (illegal weakening)
    let funcPtr: func() = example
    // OK, type signature specifies the `throws`
    let funcPtr2: func() throws = example
    // Illegal, `example` can throw ANYTHING (illegal weakening)
    let funcPtr3: func() throws IOException = example
    // OK, type signature specifies `throws` (legal strengthening for `throws Exception` -> `throws`)
    let funcPtr2: func() throws = example2
    
###### Calling a `throws` Function
Same as Swift, to call a `throws` function it MUST be in a `do {} catch {}` and MUST be prefixed with `try` to show 
what function is actually throwing.
    
    do {
        try example()
    } catch {
        print("Something went wrong")
    } finally {
        print("done")
    }
    
This has the intended effect of improving readability, most people will wrap large chunks of code into a 
`do {} catch {}` for just a couple functions that throw. Requiring `try` before the throwing call allows you to 
immediately know what functions are causing this when skimming through a piece of code.

#### Member Functions
By default member functions take `ref immut Self` as the type:
    
    struct Example {
        func member() {
            // Readonly access, `self` cannot be modified.
        }
    }
    
To make `self` writeable use the `mut` keyword before `func`:
    
    struct Example {
        mut func member() {
            // Writeable access, `self` can be modified.
        }
    }
    
Note that `mut` will require any reference you are accessing `member` on to also be `mut`.

#### Basic Contracts
Ghoul supports the most basic contracts `requires` and `ensures` on normal functions. 
`requires` states a requirement for calling the function and `ensures` states an effect.
    
    func get(index: usize)
    requires index < self.length {
        return _ptr[index]
    }
    
If `requires` or `ensures` fails the program will `panic` so use sparingly only in situations that would already cause 
the program to crash or would be considered "unsafe"

We go more in-depth on contracts in `Contracts`, including the `where` contract for templates.
