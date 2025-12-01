# The lab guarantees:

- All accesses are aligned

- No access crosses a cache block boundary

- Each `access` fits perfectly inside one block

So the “size” does not affect which block is touched, nor how many blocks are touched.

so we only need:

1. operation (L/S/M)
2. address

We ignore `size`

so for the Cache Lab, even if the block size were ridiculously tiny, you can still assume an access(4, 8,..) fits inside a single block, because the lab explicitly guarantees that no memory access ever crosses a cache block boundary.

## In hardware:

Blocks are always aligned `inherently` i.e the start addresses are always multiples of 2<sup>b</sup>.

- say b = 3, block size = 2<sup>3</sup> = 8. so let the memory address blocks:<br><br>
Addresses 0x1000 -> 0x1007 (Block 0)<br>
Addresses 0x1008 -> 0x100F (Block 1)<br>
Addresses 0x1010 -> 0x1017 (Block 2)<br>
Addresses 0x1018 -> 0x101F (Block 3) ...<br><br>
So as can bee seen the last digits of the start addresses are 0 or 8.<br><br>
So the addresses are like:-<br>
0 to 2<sup>b</sup>-1<br>
2<sup>b</sup> to 2&times;2<sup>b</sup>-1<br>
2&times;2<sup>b</sup> to 3&times;2<sup>b</sup>-1 ...

- Each block in main memory maps as exactly 1 block in the cache. say b = 4, so all addresses which have different combinations of the last 4 bits but have all the rest of the higher bits same map to the same line in a particular set. If even one bit higher than the most significant `b` bit is changed the address goes into a different set.<br><br>
Hence, many blocks can have the same tag but they wont be in the same set.<br><br>
_`t` bits are the highest so then change the last._

- Hence, having more number of `lines` in a `set` lets more memory blocks map to the same set and having more `block bits` lets more memory addresses correspond to the same block.

- Even though blocks are aligned, a single memory access(like reading a 8-byte double or a 16-byte struct) might cross a block boundary i.e the access may span across multiple blocks if block size (2<sup>b</sup>) is smaller than the data size which is the case a lot of times.<br><br>
The lab wants us to ignore the extra `hits`, `misses` and `evictions` that occur when a data spans multiple blocks.<br><br>
***This way you only check one line in one set per access.***

**In real hardware `s + b + t == m`**

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

### fgets()

`fgets()` keeps the newline that is already in the file. it reads up to and including the newline `\n` (unless the line is too long to fit in the buffer)

# t = m - (s + b)

ex:- The address 0x7ff0005c8 minimally only needs 36 bits.

But you take m = 64, formula gives extra unused high-order bits in the tag:

`t = 64 - (s + b)` <- number of tag bits

Those extra upper bits are always zero in your trace addresses (on x86-64 canonical addresses).

So even though t looks bigger than it “needs to be,” it doesn’t affect correctness.

So because each memory address has `m` bits that form `M` = 2<sup>m</sup> unique addresses, by that same logic each cache that has t tag bits has 2<sup>t</sup> unique tag addresses. So, 2<sup>t</sup> = 2<sup>m - (s+b)</sup> = addr / (2<sup>s</sup> * 2<sup>b</sup>) = addr >> (s+b)

See `6.4.1` ***Generic Cache Memory Organization*** and figure `6.25` `(b)` of csapp 3e 

The mapping is not really invented its just provided by whatever the bit pattern of the `t` bits in the memory address.

# set index of memory address

We do not really invent a mapping table. The set index is literally just the value the set bits(`s`) represents.
 
<pre>
            t bits      s bits        b bits
        +-----------+-------------+------------+
ADDRESS |           |             |            |  
    m-1 +-----------+-------------+------------+ 0
            Tag       Set index    Block offset
</pre>

`setindex` of a memory address in the cache<br>
`= (addr >> b) & ((1 << s) - 1)`

(1 << s) is Number of sets = 2<sup>s</sup> so its the total number of possible combinations of s which map into the cache.

2<sup>s</sup>-1 is the largest value that can be represented with `s` number of bits which is simply all `s` bits set to `1`.

So when we do `addr >> b`:-
<pre>
                    t bits        s bits
    +-----------+-------------+------------+
    |           |             |            |  
m-1 +-----------+-------------+------------+ 0
     All zeroes      Tag        Set index
</pre>

Now `& ((1 << s) - 1)`:- 
<pre>
                                  s bits
    +-----------+-------------+------------+
    |           |             |            |  
m-1 +-----------+-------------+------------+ 0
     All zeroes   All zeroes     Set index
</pre>


