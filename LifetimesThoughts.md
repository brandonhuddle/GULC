## Lifetimes Ramblings
I love lifetimes and want to try to implement them in to Ghoul. Even if we only solve some of the most basic problems 
from their use I still would like to have a generic way to do the smallest things such as:
    
    public func mangleRef(_ refer: ref mut i32) -> ref mut i32 {
        return refer
    }
    
    public func test() -> ref mut i32 {
        let x: i32 = 12
        return mangleRef(ref mut x)
    }
    
While the above would be easy to write a very specific way of detecting it (or detecting it after inlining) I think 
having lifetimes would be a much easier and more generic way of solving not only that super easy issue but any issue 
related to references duration.

#### Related Ideas
One thing I've been considering is `comp` members. These would be used for storing compile-time information about a 
type. An idea like:
    
    public struct Example {
        public comp var lifetime: usize = 0
    }
    
This would make every instance of `StructType(StructDecl(Example))` store a field `lifetime`.

(`comp` == "compiler time" maybe `comptime` would be better, not sure.)

The idea for these special member `comp var`s would be to create special routines also part of the `comp` type such as:
    
    public struct Example {
        public comp var lifetime: usize = 0
        public comp var mutRefCount: usize = 0
        
        // Called when `ref mut` is created
        public comp operator ref mut
        requires mutRefCount == 0 else "`Example` can only have 1 mutable reference!" {
            ++mutRefCount
        }
        
        // Called when a `ref mut Example` is destroyed
        public comp ref mut deinit {
            mutRefCount = 0
        }
    }
    
This is really rough and only had a few minutes of thought but in to it so ignore the actual routine signatures. The 
idea is to create compile time events that user-defined types can override and modify the compiler state during. 
Basically giving you the ability to not only implement Rust-style "lifetimes" but also implement Pony-style reference 
qualifiers through `comp var` etc.

There would have to be a much better API for defining the `comp var` etc. but I think the idea of keeping track of 
extra data throughout a function during analysis could lead to some interesting results. Or it could all be a fun 
exercise ending in failure and we move on.