## Properties Syntax
Properties in C# are something I like a lot. I feel they encourage the use of proper getters and setters without 
becoming "annoying". The syntax for C#/VB properties cannot be copied exactly though, for this I think the below 
syntax would be the best attempt:
    
    public property Example: int {
        get { // NOTE: These can also be marked `private`, `protected`, etc.
            return _example;
        }
        get ref {
            return ref _example;
        }
        get ref mut {
            return ref mut _example;
        }
        set {
            _example = value;
        }
    }
    
I believe the above syntax would give us coverage on everything we could potentially want.

1. There is no need for `get mut` as `get` is not settable
2. There is no need for `set mut` as `set` implies mutable
3. `ref mut` might cause confusion for some novice programmers who won't be able to understand why the `setter` isn't 
being called on their `ref mut int` result, we might want to make it unsafe, add a warning, or something.
4. `set ref` is NOT needed. The compiler should implicitly convert `set` to `set ref` where the optimization makes sense
5. `set ref mut` would lead to confusion, this should be made an actual separate function

`mut get` means `get` will write to `this`

`mut ref` means `ref` will write to `this`, returns `ref` not `ref mut`

`mut ref mut` isn't needed, `ref mut` already requires `this` to be `mut`