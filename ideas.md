## Potential Future Ideas
### `friend` and `final friend`
`friend` would make the type and all children of that type friends
`final friend` would make ONLY the type specified a friend, no other types unless more are specified
##### Examples
```
trait ExampleFriends {}

struct Example
friend ExampleFriends {
    // Any type that implements `ExampleFriends` is a friend
}

struct FinalExample
final friend Example
final friend ExampleFriends {
    // Only `Example` and `ExampleFriends` are friends. No other type is a friend and implementing `ExampleFriends` 
    // does NOT make you a friend.
}
```
##### Questions
 * Can you add a `friend` from an `extension`? Extensions have access to `private` and `protected` variables but should 
   they be allowed to share that access?
 * Should the `friend` be a member like C++ or part of the type signature like a contract?


### `defer` Statement
`defer` would be a statement which would basically give the same ability to do what `deinit` does in the background. It 
makes it so the "deferred" statement is executed before that branch of code is ended/when the scope ends.

NOTE: I believe Go has a `defer` statement which defers until `return` is called? I've rarely used Go so I can't 
      confirm how Go does it but this could be a potential confusion to Go developers. We don't defer until `return` we 
      defer until the end of scope in the same way `deinit` is called at the end of scope.
##### Proposed Syntax
 * `defer $stmt` - defers the `$stmt`, any variable calls would be converted to immut references to those variables.
 * `defer[=] $stmt` - defers `$stmt`, any variable calls will create copies to those variables
 * `defer[copy] $stmt` - possible alternative to the above?
 * `defer[move] $stmt` - possible complimentary statement to above, just doing `move` instead of `copy`
 * `defer[ref] $stmt` - any variable calls would be converted to immut references to those variables
 * `defer[ref, i] $stmt` - all variable calls are immut references EXCEPT `i` which is copied instead
 * `defer[=, ref i] $stmt` - all variable calls are copied except `i` which is an `ref immut`
 * `defer[copy, ref i] $stmt` - possible alternative to the above?
 * `defer[ref, move i] $stmt` - possible compliment to above, makes all variable `ref immut` except `i` which is moved
 * `defer[ref mut] $stmt` - same as `defer[ref] $stmt` except the references are mutable. Errors when `ref mut` isn't 
   possible
##### Examples
The function below as written with `defer`:
```
func main() {
    defer print("exited")
    
    if runtimeCheck {
        print("runtimeCheck")
        return
    } else if runtimeCheck2 {
        print("runtimeCheck2")
        return
    }

    return
}
```
The above would be converted to:
```
func main() {
    if runtimeCheck {
        print("runtimeCheck")
        print("exited")
        return
    } else if runtimeCheck2 {
        print("runtimeCheck2")
        print("exited")
        return
    }

    print("exited")
    return
}
```
Obviously this is an extremely basic example with very few branches (and the code could easily be done any code 
duplication already) but this is solely to show how it would work.

Example with `break` and `continue`:
```
func main() {
    outerLoop:
    @for i in 1...10 {
        defer print("Finished outer index {0}", i)
        
        innerLoop:
        @for j in 1...10 {
            defer print("Finished inner index {0}", j)
            
            if (i + j) == 12 {
                continue outerLoop
            }
        }
    }
}
```
The above would be converted to:
```
func main() {
    outerLoop:
    @for i in 1...10 {
        innerLoop:
        @for j in 1...10 {
            if (i + j) == 12 {
                print("Finished inner index {0}", j)
                print("Finished outer index {0}", i)
                continue outerLoop
            }
            print("Finished inner index {0}", j)
        }

        print("Finished outer index {0}", i)
    }
}
```
Notice how the `continue outerLoop` now has both print statements. This is because the `continue` is ending the current 
`outerLoop` scope. Since the scope for `outerLoop` is closing we call all "deferred" statements in the reverse order 
they were added in. So `stmt1, stmt2, stmt3` would being called in the order of `stmt3, stmt2, stmt1`

An example this could be used for in a C-style fashion (which would be unsafe and WRONG to do in Ghoul due to smart 
pointers, but that is unrelated) would be to do this:
```
func main() {
    let unsafePointer: *int = malloc<int>();
    defer free(unsafePointer)

    // Do whatever you want with as many branches and extra condintional `return` statements as you want
}
```
In the above example `unsafePointer` is guaranteed to be freed in normal, safe conditions. (NOTE: if you `abort` it may 
not be freed). This would be useful in C BUT with Ghoul we have the `shared` and other smart pointers to handle the 
pointers safely for us without the need for an unsafe raw pointer.
##### Questions
 * How do we handle `defer $stmt`? Do we copy all data for the `$stmt` like we would with a lambda? Or do we keep 
   references? I.e. does `defer print($varName)` immediately copy the data for `$varName` or does it keep a reference 
   to `$varName` and use that for the `defer`? The benefit of just keeping the reference is we don't have to worry 
   about if the data is copyable or not. The downside is `defer print($varName)` might end up doing something the 
   developer wasn't expecting. Should we instead do what C++ does with lambdas and do the `[...] {}` to allow the 
   developer to choose what they want within a capture list? E.g. `defer [&] print($varName)` and 
   `defer [=] print($varName)`. If we do the capture list we may have to change the syntax to prevent confusion to 
   both the compiler and developers. `defer []int(10)` would be valid syntax to allocate an array within the deferred 
   statement...
 * If we don't give the developer the option and instead force a copy, we could end up triggering things that wouldn't 
   make sense at first glance. If you have a type with a custom copy constructor and you do:
   ```
   let x = ValueType()
   defer x.printValue()
   ```
   It wouldn't be obvious that `x::init copy()` is being called during the `defer`. Personally, I think we need a 
   capture list either with `defer<capturelist> ...` or `defer |ref| x.printValue()` (might be confusing to those who 
   know Rust).
   It might actually be okay to just do `defer[...]` always. Error if there is no `[]` OR treat `[=]` and `[ref]` as 
   special syntax where `[=]` and `[ref]` cannot be accidentally parsed by the expression parser.


### Lambdas/Closures
Lambdas have become a core part of programming. Their syntax differs by language 
such as Swift `{ param in return param }`/`{ (param: Int) -> Int return param }`, 
C# `(param) => param;`/`(int param) => param;`, 
Rust `|param| param;`/`|param: i32| -> i32 { param }`,
C++ `[](int param) -> int { return param; }`/`[] { return 1; }`/`[]<typename T>(T param) -> T { return param; }`
##### Proposed Syntax
Personally I am most comfortable with the C++ syntax (probably obvious since I call it a "lambda" instead of "closure") 
and I feel it is the most complete and well rounded syntax out of all of them. C++'s syntax gives full granular control 
over the "capture list" for if you reference variables outside of the lambda:
    
    int i = 12;
    // i will be a `copy`
    [=] { return i; }
    // i will be a `ref`
    [&] { return i; }
    
It also is the only syntax I know of that allows template arguments:
    
    []<typename T, std::size_t length>(T param) -> T { return param; }
    
Though I can't think of a place where template lambdas make enough sense to warrant the effort required. Templates are 
required to be generated at compile time which would complicate their implementation. If anyone can provide an example 
where template lambdas make sense I would consider their addition (NOTE: Would require us to make `VariableDecl` 
templated I believe...)

It is also only one of two that I know allow return type. As such I believe we should use a "Ghoulish" version of the 
syntax:
    
    func main() -> i32 {
        // Capture by ref, 1 function parameter, return type `bool`
        let lambda = func[ref](_ param: i32) -> bool { return param > 12; }
        // `func` in an expression list will be parsed as a lambda.
        // `func` within a type area will be parsed as a function pointer type
        let lambda2: func(_: i32) -> bool = lambda
        return lambda(4) as i32
    }
    
I think any language trying to replace C++ is severely limited when missing the capture list syntax.

Capture lists are exactly the same as the proposed `defer` capture lists.
##### Questions
 * Do we want to support template lambdas? I can't imagine a scenario where they would ever be needed. They also can't 
   be passed around in their template form since the template needs solved at compile time. I think we should not 
   support template parameters, not worth the effort required. Create a template function "factory" instead to generate 
   the normal lambda while still using templates.
 * Should we look for a different syntax? As someone who has worked in C++ for most of his career I think the syntax 
   proposed is the best of both worlds. In normal execution without lambdas `func` would just error out in an 
   expression list BUT it makes the most sense to make it a lambda since it is almost the same syntax for a normal 
   `func` declaration just without a name `let main = func[](_ args: []string) -> i32 { return args.length as i32 }`
 * Is the syntax for `func[](){}` confusing? It basically is every type of "bracket" in the language exception `<>` all
   in the same area. Also `func[]` could confuse some people into either thinking that 
   1. This is declaring an operator `[]`, like the Swift syntax
   2. This is an array of `func` pointers, like the legacy C-style syntax
   