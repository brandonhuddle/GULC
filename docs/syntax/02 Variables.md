## Variables
Variables in Ghoul are very similar to Rust's variables.
    
    let x = 0 // Immutable i32
    let y = 0f32 // Immutable f32
    let mut z = 0 // Mutable i32
    
    const let i = 0 // Compile time i32, will be completely elided
    
    let j: ref i32 = ref x // Immutable reference to an i32 variable, `ref` is required here unlike for parameters
    let mut k: ref mut f32 = ref mut y // Mutable reference to an f32 variable
    // References ARE reassignable as long as the variable containing them is marked `let mut`
    k = ref mut y
    
    // NOTE: `ref` isn't needed for `ref immut` parameters, only `ref mut` requires an explicit `ref mut ...` in the 
    //       argument list. This is done because the basic semantics for calling a function with a value parameter vs 
    //       an immutable reference parameter are the same when calling. The only difference between the two is a 
    //       potential optimization with the value copy would take to long (in which case `ref immut` would be faster)
    funcName(argRef: k)
    
    let mut outValue: i32
    
    if int.parse(try: "1234", out outValue) {
        // If `parse` returns true then `outValue` was filled with the parsed value
    }
    
    // NOTE: `out` is required to always have the value set, `outValue` will be `0` here if parse failed.
