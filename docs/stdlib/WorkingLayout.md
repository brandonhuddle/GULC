## WIP Layout
I'm using this document to layout a basic "map" of what I think will be the standard library.

#### Basic Layout
    
    std.algorithm
    std.chrono
    std.collections
        std.collections.List<Element> // C++'s vector<T> or C#'s List<T>, I'm choosing the `List<T>` name over 
                                      // `Vector<T>` due to 1. `List<T>` being a better descriptor for it and 
                                      // 2. Even Alexander Stepanov, the person who named `vector<T>` in C++, has 
                                      // admitted to using the name `vector<T>` being a mistake. I feel `List<T>` still 
                                      // falls in line with his idea on being humble and using terms already in use 
                                      // since C#, Java, etc. all use the name of `List<T>` for this same concept.
        std.collections.LinkedList<Element> // C++'s list<T> or C#'s LinkedList<T>
        std.collections.Dictionary<Key, Value> // C++'s map<T1, T2> or C#'s Dictionary<TKey, TValue>
        std.collections.Set<Element> // C++'s set<T>, C#'s HashSet<T>, or Swift's Set<Element>
        std.collections.Queue<Element>
        std.collections.Deque<Element> // Double ended queue (C++ deque<T>)
        std.collections.Stack<Element>
        std.collections.StaticArray<Element, const length: usize> // Might be renamed, C++'s `array<T, size_t length>`
    std.crypto // Maybe or maybe not. This would be a bad "role your own" solution of mine... Sounds really fun but should never be used in any production setting obviously.
    std.graphics // Maybe, might become `std.gfx`
    std.gui // Maybe, might be `std.graphics.gui`
    std.io
        std.io.ReadStream // Maybe, might become `std.io.InputStream`
        std.io.WriteStream // Maybe, might become `std.io.OutputStream`
        std.io.StreamReader // Abstract class for reading streams
        std.io.StreamWriter // Abstract class for writing streams
        std.io.UTF8Reader // Implements `StreamReader` with UTF8 encoding
        std.io.UTF8Writer // Implements `StreamWriter` with UTF8 encoding
        std.io.filesystem // This wouldn't be a namespace but a library name..
            std.io.Path
            std.io.Directory
            std.io.File
        std.io.net // Might change, will implement `http`, `tcp`, etc.
            std.io.net.HttpInputStream // Name undecided
            std.io.net.HttpOutputStream // Name undecided
    std.math
    std.threads
    std.security
        std.security.permissions // Might change to `std.permissions`. Generic library for managing OS permissions like requesting access to internet, access to camera, etc.
    std.platform // Maybe?
        std.platform.Architecture // x86_64, RISCV64, ARM32, ARM64, etc.
        std.platform.OperatingSystem // Linux, Windows, MacOS, Web?
        std.platform.ABI // Alternative to Architecture or compliment to it? Can't decide yet.
        
    
There is still a lot undecided which I will plan out more once I start working on the implementation but this covers 
what I've been thinking so far.

I would LIKE to implement a `std.crypto` as I feel cryptography is becoming ever more important by the day from 
connecting to a website to sending P2P communication. As someone who enjoys DIY programming I would make the initial 
implementation entirely by hand (role-your-own-crypto...) but if Ghoul ever got any interest outside of my own hobby 
we would need to ensure there is an `openssl+std.crypto` or similar using a battle-tested cryptography library on the 
backend until the Ghoul-only have been validated. Or maybe the openssl backend will be permanent. Who knows. Can't have 
a crypto library if noone every makes one...

For graphics, it should only implement basic image formats, video formats, and a skia/cairo vector graphics API.
Vulkan will not be apart of the standard. I'm considering making a Vulkan-esque graphics API for fun that would allow 
Vulkan, WebGL 2.0, WebGPU, Metal, and DX12 to all be able to be tied into the single API with varying levels of control.
Sounds like a fun (time consuming) project that could pay off a lot in the end.

Permission systems need to be properly supported. iOS and Android both allow you to request permission of something 
right before you use it. The Ghoul OS will work the same way so we will need a generic `std.permissions` library for 
handling the permission system.

For IO I'm still undecided (I'll need to test a few things once I start implementing) but the current idea is to go the 
`trait` route with `ReadStream::func read(...)`, `WriteStream::func write(...)`, etc.
Then you would use `UTF8Reader`/`UTF8Writer` for passing streamable objects around.
    
    let file = File.open("example.txt")
    let reader = @new UTF8Reader(file.inputStream)
    
