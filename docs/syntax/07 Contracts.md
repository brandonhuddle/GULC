## Contracts
Inspired by Midori, Ghoul supports basic contracts. The only contracts being `requires` for precheck, `ensures` for 
postcheck, and `where` being a precheck for template parameters that give templates basic properties.

Ghoul is not meant to be a "Code by Contract" language. It is meant solely to be another easy check to validate your 
code at compile time in situations that would already cause runtime errors and attempts to move as many of those 
runtime errors to the compile time as possible.

A great example of where `requires` would be useful is in wrapping raw pointers. When you allocate a pointer you 
usually know the size of the pointer from the index size and allocation size. With `requires` you can route all 
accessors of the pointer through a check to make sure the index is within the range at both compile time and run time. 
At compile time if the compiler knows exactly what the `index` is it can run it through the contracts and error out if 
any of them fail. Before the program ever runs.

The `ensures` contract can also allow various checks to be completely elided.

NOTE: Contracts in Ghoul are NOT a replacement for basic testing. You should still be writing unit tests, etc. 
      Contracts are supplements not replacements.

Like Midori, a runtime contract failure will panic or "abandon" the process.

TODO: Should we give the option to NOT panic? I could see the use of having SOME contracts `throw` in some situations.
      Maybe `requires ... else throw Exception()` and `ensures ... else throw Exception()` or something?

Contracts should do their best to describe data flow. Doing so properly will allow the compiler to elide any 
unnecessary checks while still guaranteeing the safety described within the contracts.

#### Examples
    
    struct SafePtr<T> {
        unsafe var rawPtr: *T
        
        init()
        ensures rawPtr != null {
            rawPtr = malloc<T>()
        }
        
        operator prefix *() -> &T
        requires rawPtr != null
        ensures rawPtr != null {
            unsafe {
                return *rawPtr
            } 
        }
    }
    
    let x = SafePtr<i32>()
    
    // NOTE: This if condition and check will be removed entirely, `x.rawPtr` is guaranteed to NOT be null due to the 
    //       `ensures` attached to the `init`
    //       The `requires rawPtr != null` will also be ignored due to the `init` guarantee. `init` cannot return 
    //       unless `rawPtr` isn't null. If it is null `ensures` will panic.
    if x.rawPtr != null {
        print("Value: {0}", *x)
    }
    
