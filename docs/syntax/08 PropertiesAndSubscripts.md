## Properties & Subscripts
One feature that becomes extremely useful when you use it for a while is the concept of "properties". Properties in 
Ghoul are implemented very similarly to C# with the main differences being the `prop` keyword and the addition of more 
than just the dual `get` and `set`.

Example:
    
    struct Example {
        private var underlyingValue: i32
        
        prop PropertyName: i32 {
            // Called when accessing the value normally
            public get {
                return underlyingValue
            }
            // Called only where the reference to `Example` is `mut`, not always needed
            protected mut get {
                return underlyingValue
            }
            // Called when trying to `ref` the `PropertyName` 
            private get ref {
                return ref underlyingValue
            }
            // Called when trying to `ref mut` the `PropertyName`
            internal mut get ref mut {
                return ref mut underlyingValue
            }
            // Called when setting the property
            protected internal set {
                underlyingValue = value
            }
        }
    }
    
The above is one of the most extreme cases where you want to cover every possible base in Ghoul. More than likely your 
usual use case will only require:
    
    prop PropertyName: i32 {
        get {}
        set {}
    }
    
Which would cover the most basic cases.

#### Examples
Auto-generate `var`:
    
    struct Example {
        prop Property: i32 {
            get
            set
        }
    }
    
The above will automatically generate an underlying `var` for the property and generate the `get` and `set` body for 
that `var`, same as in C#.

It should also be noted that `prop` can appear outside of a data structure:
    
    namespace Example {
        static prop GlobalProperty: f32 {
            get
            set
        }
    }
    

Properties also can have `throws`, `requires`, and `ensures` in the same way all functions use them:
    
    prop Example: i32 {
        get throws {
            throw NotImplementedException()
        }
        set
        requires underlyingValue != null
        // Access the hidden `value` parameter
        requires value != null
        ensures underlyingValue != null {
            underlyingValue = value
        }
    }
    

#### Edge Case Support
There may be times when you want to give `ref mut` access to the property, this is supported as a niche case in Ghoul 
with `mut get ref mut`. The breakdown for that is:
 1. First `mut` means `self` must be `mut`
 2. `get` makes this a getter
 3. `ref mut` states that this is the handler for when `ref mut` is called

Now you could do `get ref mut` and that would be perfectly valid syntax BUT if you can't grab a `ref mut` to the 
underlying `var` you will have an error when returning.

##### NOTES
For `set` you don't need to specify `mut` since `mut` is inferred by the fact that you are setting the value.