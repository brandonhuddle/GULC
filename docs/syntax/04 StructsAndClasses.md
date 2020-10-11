## Structs & Classes
Structs and classes are almost identical how they act in C++ with the notably difference being no multiple inheritance 
of other structs and `immut` by default for functions.

The other notable difference between C++ and Ghoul classes is that `struct` does NOT store its type for RTTI. `class` 
is always polymorphic (`virtual deinit` always) and will always store the type of class in its `vtable`.

Unlike Rust, Ghoul does support inheritance though it is recommended to be used sparingly. Not all situations need to 
have inheritance and it is recommended to use `trait` in nearly all situations inheritance would be used instead.

The main reason inheritance is still supported is because no one paradigm fits all. While it can be overused, competent 
developers can use it to make libraries that are much easier to write, maintain, and read for SOME problems.

Multiple inheritance is not supported due to complexity, basic problems caused by it, and because `trait` should cover
nearly all use cases of MI.

#### Examples
    
    struct vec2 {
        var x: f32
        var y: f32
        
        init() {
            x = 0
            y = 0
        }
        
        init(x: f32, y: f32) {
            self.x = x
            self.y = y
        }
    }
    
    // Note the lack of `public`, we don't support `private` or `protected` bases in Ghoul
    struct vec3: vec2 {
        var z: f32
        
        // I'm most used to C# in this regard so I've chosen `base` instead of `super`
        init() : base() {
            z = 0
        }
        
        init(x: f32, y: f32, z: f32) : base(x: x, y: y) {
            self.z = z
        } 
    }
    

##### Call Operator/Functor
    
    struct Func {
        // No `func` needed
        call() {
        
        }
    }
    
    // Usage:
    let callable: Func = Func()
    callable() // This will call `call()`
    
##### Subscript Operator
    
    struct List<T> {
        private var rawPtr: *T
        public prop length: usize {
            get
            get ref
            private set
        }
        
        // NOTE: `subcript` can have multiple parameters AND argument labels.
        subscript(_ index: usize) -> &T
        requires index < length {
            unsafe {
                return rawPtr[index]
            }
        }
    }
    
    let example = List<i32>()
    // Calls `subscript`
    example[12]
    
##### Destructor
NOTE: For `class` the `deinit` is `virtual` by default and cannot be made `final`.
      For `struct` the `deinit` is `final` by default but can be changed to `virtual`.
    
    struct Example {
        // NOTE: Cannot be `private` or `protected`
        //       CAN be `virtual` and will automatically inherit `virtual` if the base is `virtual`
        deinit {
            // Do anything, all variables with destructors will happen automatically at the end.
        }
    }
    
#### Shadow, Overload, Override, etc.
With us supporting inheritance we need to account for a wide range of potential issues. With Ghoul you can handle every 
niche case scenario with ease:
    
    struct BaseStruct {
        virtual func print() {
            std.io.console.print("BaseStruct says hello")
        }
    }
    
    struct ChildStruct: BaseStruct {
        // `final` marks this as the final possible virtual definition
        final override func print() {
            // Call the base's print definition
            base.print()
            std.io.console.print("ChildStruct also says hello")
        }
    }
    
    struct GrandchildStruct: ChildStruct {
        // NOTE: Since `base.print` is `final` this is an entirely new entry into the vtable
        @shadows
        virtual func print() {
            base.print()
            std.io.console.print("GrandchildStruct also says hello")
        }
    }
    
    struct GreatGrandchildStruct: GrandchildStruct {
        // This is an explicit definition for the `vtable::print`
        override func GrandchildStruct::print() {
            base.print()
            std.io.console.print("GreatGrandchildStruct also says hello")
        }
        
        // This shadows the `vtable::print` and creates a non-virtual `print` function. When the variable type is 
        // `GreatGrandchildStruct` `print` will be called, when casted to any parent the `vtable::print` will be 
        // called.
        @shadows
        func print() {
            std.io.console.print("GreatGrandchildStruct says goodbye")
        }
    }
    
The above is a bit confusing but solves an issue that can be run in to in some languages:
 1. The language provides the ability to `shadow` a member
 2. The language provides the ability to `override` a member
 3. The language forgets to give the ability to provide both at the same time

It is very niche to need it and you should always try to avoid it but when you need it you'll notice just how limited 
the language you're using is. Can be overcome in most languages by another type into the inheritance tree to do the 
`override` before its child does the `shadow` but I don't think we should need that.

Just like in `Traits`, if you implement two traits that define the same declaration you can specify the implementation 
per the type with the same syntax.

NOTE: `@shadows` is just my current thought for how to remove the `shadows` warning, this may change within the 
      standard library definition.

#### Runtime Types
In Ghoul creating a `class` will automatically store the `typeid` within the `class::vtable` to allow for runtime type 
checking.

`struct` will never store the `typeid` within the `struct::vtable`.

    
    class View { ... }
    class Window: View { ... }
    
    let rootView: View = Window(title: "Hello, World!")
    
    // NOTE: This syntax is undecided...
    // Since `View` is a `class` we will check `View::vtable` for type information to validate this cast at runtime.
    if let window = rootView as? Window {
        
    }
    
    if rootView is Window {
        print("rootView is a Window!")
    }
    
Though `struct` cannot contain type info within its `vtable` you can still hold type information about any type using a 
`box<T>`. `box<T>` stores the typeid of `T` that it was constructed with:
    
    // Note the struct instead of class...
    struct View { ... }
    struct Window: View { ... }
    
    let rootView: box<View> = box(Window(title: "Hello, World!"))
    
    // NOTE: This syntax is undecided...
    // Since `box<T>` overloads the `as`, `is`, etc. operators it is able to change this check to validate against 
    // `box<T>::typeid`. In turn allowing you to store and check the runtime type information about a struct.
    if let window = rootView as? Window {
        
    }
    
One potential "gotcha" about `box<T>` is that it stores the type it was _constructed_ with. What that means is if you 
do:
    
    let window: ref View = Window(title: "")
    let boxedView: box<View> = box<View>(window)
    
The `box<T>::typeid` will be `View` because `typeof(window) == ref View` not `ref Window`.
