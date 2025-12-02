# The lab guarantees:

- All accesses are aligned

- No access crosses a cache block boundary

- Each `access` fits perfectly inside one block

So the “size” does not affect which block is touched, nor how many blocks are touched.

so we only need:

1. operation (L/S/M)
2. address

We ignore `size`

so for the Cache Lab, even if the block size were ridiculously tiny, you can still assume the data of an access(4, 8,..) fits inside a single block, because the lab explicitly guarantees that no memory access ever crosses a cache block boundary.

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

# tag

ex:- The main memory address 0x7ff0005c8 minimally only needs 36 bits.

But you take m = 64, which gives extra unused high-order bits in the tag:

`t = 64 - (s + b)` <- number of tag bits

Those extra upper bits are always zero in your trace addresses (on x86-64 canonical addresses).

So even though t looks bigger than it “needs to be,” it doesn’t affect correctness.

tag = addr >> (s+b) because the `t` bits are the highest. 
<pre>
            t bits      s bits        b bits
        +-----------+-------------+------------+
ADDRESS |           |             |            |  
    m-1 +-----------+-------------+------------+ 0
            Tag       Set index    Block offset
</pre>

Shifting a Uint_t right pads the MSBs with 0 which means after shifting right by (s+b) only the tag bits remain.
<pre>
                                  t bits
    +-----------+-------------+------------+
    |           |             |            |  
m-1 +-----------+-------------+------------+ 0
      All zeroes   All zeroes      Tag
</pre>

See `section 6.4.1` ***Generic Cache Memory Organization*** and figure `6.25` `(b)` of csapp 3e 

The mapping is not really invented its just provided by whatever the bit pattern of the `t` bits in the memory address.

The fact that we extract tag, setindex and the blocks directly from the memory address itself is cool :)

# set index

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

# memory blocks

Because all the addresses in the same block would have the same tag and set index, we do not need to keep track of which block a particular address belongs to.

So if block bits `b` = 5 then 2<sup>5</sup> different addresses are automatically in the same block.

# LRU

Cyclic LRU scheme:

- Each line has an `age`, incremented on access.
 
- Each set has an `increasing` flag.
 
- Ages are bounded only by the type (e.g., uint8_t), and when one line overflows:
 
- Flip `increasing` → eviction logic switches between picking min age or max age
 
- Eviction: pick min/max age depending on flag.
 
- Effectively: per-set cyclic LRU with controlled overflow.

**Problem:** 

The scheme assumes all lines get accessed roughly equally between evictions.

Since a line's age is incremented everytime its `accessed` and not everytime its evicted, the hot lines which are accessed very frequently will wrap around and **not wait for the other lines to catch up to its age.** So the `increase` flag wont work.

**Solution:**

Bound counters per line between 0 and MAX 

- Each line’s age goes 0 → max 

- Increment on access, but cap at `max`.<br><br>
Evict the line with the smallest age.

- when all lines reach max age, flip `increasing` flag to 0.<br><br>
Decrement on access, but cap at min(`0`).<br><br>
Evict the line with the largest age.

- When all lines reach 0 → flip `increasing` to 1 again.

This creates a cyclic “bounded `LRU”`

#### Note:

For a simulator: scanning all lines is fine, even for 16-way or more.

If you were designing actual hardware, you’d switch to PLRU or tree-based approximation once associativity is large.

The only special thing about `M` operation is that it counts as **two accesses**(a load then a hit), so it always updates LRU twice and can produce `2` hits and `0` miss or `1` hit and `1` miss but can never give `0` hits because the store after the load is always a hit.


