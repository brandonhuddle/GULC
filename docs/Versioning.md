## Versioning
This is just a temporary document for me to write down my thoughts on how I think Ghoul should be version. Both C and 
C++ use a year for their versions, Swift uses a major number version, Java uses their minor version number, etc. 
I think for Ghoul we should do the following:
    
    major.minor.revision.patch
    
With the following characteristics:
 * Major - this is the overall version for the current Ghoul standard. This is equivalent to Swift 5, C++ 20, etc. 
   This would allow API breaking changes, ABI breaking changes, API additions, etc. We could throw out the old 
   standards and replace them (but probably would never do such a thing, I just feel this should be allowed)
 * Minor - this is an ABI breaking change to the `major` standard. This would be something like reorganizing a struct, 
   adding new non-extension fields to a class/struct/union, adding a new parameter to a function with a default value, 
   etc. This should require a recompile to work with but ALL SOURCE WRITTEN FOR `major` should still compile. Only the 
   ABI should be broken but a recompile should be a guaranteed fix.
 * Revision - this is for added APIs that don't break old API or ABI. ALL SOURCE WRITTEN FOR `major.minor` should still 
   compile. ALL BINARIES WRITTEN FOR `major.minor` should still run WITHOUT needing to recompile. Neither API nor ABI 
   can be broken.
 * Patch - bug fixes that don't break API nor ABI. This is something like removing an accidental divide by zero etc.

I very much believe `major` should be able to break API and ABI. Limiting ourselves as hard as C++ does will lead us 
into the mess that C++ has become. We should obviously have rules such as you deprecate something for a few years, then 
move it to a compat library, THEN delete it entirely. But if something would be more efficient if we changed it to a 
reference, our implementation of `hash` could be sped up, etc. we should be able to do that. All programs written for 
`major` will continue to work but to move from one `major` to another `major` version will require more work than just 
compiling under the new version. You will need to update to use the new APIs.

A good case for `major` is in 10 years we may find that `std.gui` or `std.graphics` don't match the way modern GUIs or 
modern graphics cards work. We shouldn't continue to support them for legacy applications. They should be deprecated, 
replaced with modern solutions, have a `compat` library built to use the modern solution if possible, and then 
completely removed.

With this versioning you would include version as such:
    
    // This would allow 1.1.0.* to 1.1.9999.*
    std.io >= 1.1.0
    // This would allow 1.0.*.* to 1.9999.*.* for compiling but would lock you in to 1.x version you compiled with
    std.io >= 1.0
    // This isn't allowed, minor version is still important
    std.io >= 1
    // This also isn't allowed, patches can't be locked on
    std.io == 1.1.1.14
    
