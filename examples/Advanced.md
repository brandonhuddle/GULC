## Advanced Operations To Consider
 * Support "precedence" and "associativity" for showing a "reorder strategy" or "transform strategy"
 * Auto-expression lists (i.e. `operator infix +(right: Self) -> Self` == `operator infix +(item: Self...) -> Self`)
   expression lists like the above are especially needed optimizations 
   (e.g. converting `"Long" + " concatenation" + " string" + " takes " + "forever"` to 
   `+("Long", " concatenation", " string", " takes " , "forever")`)