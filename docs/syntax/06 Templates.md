## Templates
Ghoul supports the full power of C++ templates in the syntax of Swift and C#. With Ghoul there is no `template` keyword 
or `typename`. To declare a template value type you use `<const name: type>` if `const` is not specified then it will 
be considered a type declaration, e.g. `<T>` is a type.

The biggest difference between Ghoul and C++ is what you can do with a template type. Unlike in C++ where you can pass 
`T` anywhere, use any operator on `T`, and just do anything with `T` until instantiation; in Ghoul you have to describe 
what `T` can do with a `where` contract.

#### Examples
    
    // The `where T : Addable<T, T` specifies that `T` implements the `Addable<T, T>` trait giving it support for 
    // `operator infix +(_: &T) -> T`
    struct Sum<T>
    where T : Addable<T, T> {
        call(_ array: &[]T) -> T {
            let mut result: T = T()
            
            @for index in array {
                result += index
            }
            
            return result
        }
    }
    
    // You could also do to specify the operator and default constructor explicitly (but you should use traits instead 
    // to simplify code and decrease overall duplication. 
    struct Sum<T>
    where T has operator infix +(_: &T) -> T
    where T has init() {
        // Same as above.
    }
    
In the above example, even though `operator infix +` exists for the type `operator infix -` does not so you CANNOT use 
`-`. The only thing you can do without specify it in a `where` is pass around the reference to that type.
    
    struct Array<IndexType, const length: usize>
    where length > 0 {
        private var flatArray: [IndexType; length]
        
        subscript(_ index: usize) -> ref IndexType
        requires index < length {
            return ref flatArray[index]
        }
    }
    
The above uses a `const` template parameter with a `where` clause. This can be use as `array<i32, 1>`. 
`array<i32, 0>` will be caught by the compiler as an error specifying it doesn't meet requirement `where length > 0`

Notice the `return ref flatArray[index]` instead of `return flatArray[index]`, there is no description of a `copy` or 
`move` constructor in the `where` contracts so we can't access the indexes by value, only reference.

Through `const func` you will also be able to define custom `where` contracts.

Through `const struct`, `const class`, `const union`, etc. you will also be able to define your own `const` template 
parameters too. `struct Example<const x: conststring>` being something that may be handy, a basic `const` string.
