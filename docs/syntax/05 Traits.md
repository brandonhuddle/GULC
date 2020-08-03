## Traits
Traits are very similar to Rust's traits or C#'s interfaces. You CANNOT add variables to a trait but you can add 
properties, methods, and constructors. You CANNOT specify `virtual` for any members, the decision to make something 
`virtual` is up to the struct implementing the `trait`. Traits DO NOT have a unique `vtable`. The `trait` might have a 
`vtable` in implementation but `struct` only has a single `vtable`, it will not contain more than one `vtable`.

Because `trait` cannot add variables and can't specify `virtual`, you can use `extension` to implement a trait on any 
type. We will explore the basics of that here and go more in-depth in the `Extensions` page.

#### Basic Examples
    
    trait Addable<RightType, ResultType = Self> {
        operator infix +(_ right: ref RightType) -> ResultType
    }
    
    trait DefaultExample {
        func print() {
            std.io.console.print("Traits can also define default implementations")
        }
    }
    
    trait PropExample {
        // Implementor is required to provide a `prop length` with both a `get` and `set`
        // Implementor is allowed to also specify `get ref` etc. if they want as it won't affect the trait
        prop length {
            get
            set
        }
    }
    
    trait DefaultConstructor {
        // The below tells the implementor that they are required to have default constructor `init()` to adhere to the 
        // trait. Constructors CANNOT have default implementations. TODO: Or should they?
        init()
    }
    
    // Extensions can be used to add any trait to any type
    extension i32: Addable<i32> {
        // NOTE: We don't need to define this as it is already defined for `i32` but I've written the syntax anyway to 
        //       show how to adhere.
        //public infix +(_ right: ref i32) -> i32 {}
    }
    
#### Semantics
Traits should mainly be used for constraining template types but when needed outside of templates they can be casted to 
(but not from, unless explicitly defined).

Examples:
    
    trait Subtractable<RightType, ResultType = Self> {
        operator infix -(_ right: ref RightType) -> ResultType
    }
    
    extension i32: Subtractable<i32> {
        // NOTE: No need for explicit implementation, already defined
    }
    
    let x: i32 = 3
    let y: Subtractable<i32> = x // OK, `i32` implements `Subtractable<i32`
    let z: i32 = y as i32 // ERROR: `Subtractable<i32>` does not have a valid conversion to `i32`
    let w: i32 = y // ERROR: Same as above, neither explicit or implicit are defined for this conversion.
    
In the above there is no conversion to any types that implement a trait. To support that you would have to add your own 
overloaded cast operator to support it.

#### Inheritance
Traits in Ghoul can extend other traits. Unlike Rust these inherited traits do not have to be repeated by the type 
implementing the child trait.

Examples:
    
    trait BaseTrait1 {}
    trait BaseTrait2 {}
    trait ChildTrait: BaseTrait1, BaseTrait2 {}
    
    // This will be noted as implementing `ChildTrait`, `BaseTrait1`, and `BaseTrait2`
    struct ChildStruct: ChildTrait {}
    
    // Also legal but will output a warning saying `BaseTrait2` is redundant as `ChildTrait` already implements it.
    struct ChildStruct2: ChildTrait, BaseTrait2 {}
    
#### Multiple Declarations
Sometimes multiple traits will define the same function. The default reaction to that would be to just combine them in 
the implementor, but in the scenario that that is impossible (say, both have default implementations) you can provide 
explicit implementation like so:
    
    trait BaseTrait1 {
        func print() {
            std.io.console.print("BaseTrait1 says hello")
        }
    }
    trait BaseTrait2 {
        func print() {
            std.io.console.print("BaseTrait2 says hello")
        }
    }
    
    trait ChildTrait: BaseTrait1, BaseTrait2 {
        func BaseTrait1::print() {
            // Just call the base default implementation...
            BaseTrait1::print()
        }
        
        func BaseTrait2::print() {
            // Call base default implementation...
            BaseTrait2::print()
        }
        
        func print() {
            BaseTrait1::print()
            BaseTrait2::print()
            
            print("ChildTrait also says hello")
        }
    }
    
The above is just a very basic example of how to fix the most extreme niche case example for traits. But this niche 
case does appear after any extended amount of time with any level of inheritance and needs proper handling.

TODO: We might support `= default` at some point so you can do `func BaseTrait1::print() = default` for less verbosity.
