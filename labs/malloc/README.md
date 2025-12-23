
# Blocksize

The payload needs to be 16-byte aligned. Not just for making the block size 16-byte aligned which could have been done by just tightly packing the header and the payload with using only enough leading or trailing padding to make block size multiple of 16 so that when we store that value in the header we can use the last 4 bits of the block size value to store flags.

**Two separate constraints**

There are two separate reasons for the 16-byte alignment in malloc-style allocators:

- Payload alignment → the start address of the payload must be a multiple of 16 for ABI/CPU correctness.

- Metadata tagging → the block size must be a multiple of 16 so the low bits are free for flags.

These are related but independent. Just rounding block size doesn’t fix payload alignment.
- Block size is a multiple of 16 → flags OK
- Payload start is misaligned → crashes on SIMD or long double

## Trade-offs implicit and explicit lists:
<pre>
 Feature	                        Implicit Free List	                    Explicit Free List
---------                          --------------------                    --------------------
 
 Metadata overhead per block	    16B (header + footer)	                24B (header + prev + next)
 
 Allocation speed	                O(n) scan entire heap	                O(1)-O(n) scan free list only
 
 Freeing & coalescing	            Slow (may need footer)	                Fast (prev/next pointers)
</pre>

A free block in explicit list:-
<pre>
HEADER
┌───────────────────────────────┐
│ size + flags (8B)             │
├───────────────────────────────┤
│ prev pointer (8B on 64-bit)   │ ← only if free
├───────────────────────────────┤
│ next pointer (8B)             │ ← only if free
└───────────────────────────────┘

UNUSED SPACE 
</pre>

Allocated block:

- Only the header (size + flags) and optionally footer exist

- Prev/Next pointers are NOT present, because the block is allocated and not in the free list

- Payload comes immediately after the header (aligned as required)

Allocated block in explicit/implicit list:-
<pre>
┌───────────────────────────────┐
│ size + flags (8B)  (HEADER)   │
├───────────────────────────────┤
│ padding if required           │ ← aligns payload
├───────────────────────────────┤
│       PAYLOAD                 │ ← returned to user
├───────────────────────────────┤
│ padding if required  (slop)   │ ← rounding to 64B total block size
└───────────────────────────────┘
</pre>



