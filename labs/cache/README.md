# The lab guarantees:

- All accesses are aligned

- No access crosses a cache block boundary

- Each access fits perfectly inside one block

So the “size” does not affect which block is touched, nor how many blocks are touched.

so we only need:

1. operation (L/S/M)
2. address

We ignore `size`

so for the Cache Lab, even if the block size were 2 bytes (ridiculously tiny), you can still assume a double fits inside a single block, because the lab explicitly guarantees that no memory access ever crosses a cache block boundary

# What atoi does

`int atoi(const char *str);`

- Converts the initial part of the string str to an integer.

- It ignores leading whitespace.

- Then it parses an optional + or -.

- Then it converts consecutive digits (0–9) until a non-digit is encountered

<pre>
atoi("abc");    // not a number
atoi("   xyz"); // leading whitespace ignored, still not a number
atoi("!123");   // not a number
</pre>
In all these cases, atoi will return 0.

It does not signal an error if conversion fails.
 
It simply stops at the first non-digit character and returns the accumulated integer (which is 0 if there were no digits).

<pre>
atoi("42abc"); // returns 42
atoi("-123xyz"); // returns -123
atoi(" -42"); // returns -42
</pre>
Only the initial numeric part is used. The rest is ignored.

Also leading whitespace is ignored.

