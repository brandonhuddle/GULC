## Error Handling
Inspired by Midori, the Ghoul error handling model relies on the idea that there are three possible error types:
 1. Expected error - an error that non-exceptional and is recoverable. E.g. `int.parse()` is expected to error for 
    unknown input.
 2. Exceptional error - an error that is exceptional (not expected) but is still recoverable. E.g. `file.read()` isn't 
    expected to error out, the file should already be created and all systems working. Read failing would be an 
    exception circumstance.
 3. Unrecoverable - an error that is not recoverable. These are errors you may know are possible but should already be 
    checking for prior to them being triggered. E.g. out of memory, index out of bounds, null reference, etc.

The idea of this error model is that not all errors are the same. Some are expected to be handled immediately, some are 
exceptional enough to return to a recovery point, and some are unrecoverable and require the process to abandon or 
`panic`.

#### Examples
In this section I'll give a few more examples of when each error type should be used.
##### Expected Error Examples
Expected errors are basically the same model that Rust uses. Rust treats every recoverable error as an expected error 
that requires the programmer to always handle when calling the function that could potentially cause an error.
    
    let x: i32
    
    if int.parse(try: "qwe", out x) {
        // Successful parse
    }
    
Parsing a number is dependent on external and uncontrollable input. Errors are expected to happen and should be 
handled immediately.

##### Exceptional Error Examples
Exceptional errors are the same model Swift, C#, C++, etc. use: throw an exception on error to return to a recovery 
point (i.e. the `catch` block). These errors occur in areas where an error CAN happen but the error should be so rare 
it is completely unexpected (i.e. "exceptional").
    
    do {
        // We `try` to open the response stream because stuff happens, maybe the network disconnects or the server went 
        // down. These things are exactly recoverable in the sense that you can't set a different value for `steam`, 
        // you instead return to the designated recovery area.
        let stream: Stream = try httpClient.openResponseStream()
        
        // We `try` again here because stuff happens, the server could have disconnected since we opened the stream. 
        // We return to the recovery area and cancel executing the rest of the `do` body.
        let body: string = try stream.read(size: 12)
    } catch {
        // Recover from the exception
        // I'm not including the exception type here for compactness.
    }
    
##### Unrecoverable Error Examples
Unrecoverable errors are just that, errors that can't be recovered from. These would be things you `assert`, contracts 
that must be met, etc.
    
    let x: i32 = 0
    // This would `panic` due to `x` being zero. Divide by zero is unrecoverable, if you want to prevent the panic test 
    // to make sure `x` isn't `0`
    // NOTE: The compiler can easily detect the error in this example and would error at compile time instead of 
    //       runtime, this is just an easy example.
    let y: i32 = 1 / x
    
    let arr: []i32 = [
        1, 2, 3
    ]
    
    // This would `panic` due to `12` being greater than `arr.length`, if you want to prevent this check to make sure 
    // your index isn't greater than the length of the array.
    // NOTE: This also would be caught by the compiler through the contracts within `[]i32`, there is a contract 
    //       verifying `index < length` and since `12` is known at compile time and `length` is known at compile time 
    //       the compiler would see this and error out. This is just an example.
    arr[12]
    
    // This would only `panic` if there isn't enough memory left for this allocation. This is an unexpected and 
    // unrecoverable error that requires preplanning to prevent and attempt to recover from before the error ever 
    // occurs. This also cannot be detected at compile time for obvious reasons.
    let notEnoughRam: *i32 = malloc<i32>(1000000000000)
    let nullPtr: *i32 = null
    
    // This would `panic` due to `nullPtr` being `null`, this is unrecoverable. Check to make sure the pointer isn't 
    // null before dereferencing.
    // NOTE: This would be caught by the compiler since `nullPtr` is known to be null at compile time.
    *nullPtr = 12
    

##### Mixed Cases
There may be cases where there is a mix of all types of errors in the same function. A good example from my own 
experience would be managing a `VkQueue` with Vulkan:
    
    do {
        switch try presentQueue.present(swapchain, imageIndex, semaphore) {
            case .success:
                break
            case .suboptimal, .outOfDate:
                recreateSwapchain()
            case .fullscreenExclusiveModeLost:
                // Do something with that...
        } 
    } catch VkError.deviceLost {
        // These both are exceptional cases. Losing the device or surface is not expected and is only an exceptional 
        // case that requires special handling. It may be possible to reaquire the device or surface.
    } catch VkError.surfaceLost {
        
    }
    
The above example could trigger an "expected error" which is returned by `present`, it could trigger an 
"exceptional error" which is caught in the `catch` blocks, and it could even `panic` for `VK_ERROR_OUT_OF_HOST_MEMORY`

Due to `.deviceLost` and `.surfaceLost` being exceptional but recoverable we use exceptions to handle them. A developer 
working with the Vulkan API then has the option to either attempt to reestablish the device or surface OR trigger their 
own panic for this error.

#### Panic Handler
The plan right now is to allow for handling panics similar to Rust. You'll be able to be notified of a panic and do 
whatever you want before exiting. Triggering a second panic within the panic handler will immediately abort the program.

The panic handler will have three configurable states:
 1. Full stack unwinding, know what went wrong and where it went wrong.
 2. No stack unwinding, know what went wrong but not where.
 3. Abort, don't know what went wrong or even that anything went wrong. The program terminates immediately.
