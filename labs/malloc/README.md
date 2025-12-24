
# Blocksize

The payload needs to be 16-byte aligned. Not just for making the block size 16-byte aligned which could have been done by just tightly packing the header and the payload with using only enough leading or trailing padding to make block size multiple of 16 so that when we store that value in the header we can use the last 4 bits of the block size value to store flags.

**Two separate constraints**

There are two separate reasons for the 16-byte alignment in malloc-style allocators:

- Payload alignment → the start address of the payload must be a multiple of 16 for ABI/CPU correctness.

- Metadata tagging → the block size must be a multiple of 16 so the low bits are free for flags.

These are related but independent. Just rounding block size doesn’t fix payload alignment.
- Block size is a multiple of 16 → flags OK
- Payload start is misaligned → crashes on SIMD or long double

There can be cases where the blocks would require no extra padding or slops at all(payload start addr is multiple of 16, blocksize(header + ptrs + pyload size) = multiple of 16). 

But if we store the flags as 1-2 separate bytes, they would no-doubt cause misalignment of payload start addr and so padding would be required. 

## Trade-offs implicit and explicit lists:
<pre>
 Feature	                        Implicit Free List	                    Explicit Free List
---------                          --------------------                    --------------------
 
Metadata overhead per block	        16B (header + footer)	                24B (header + prev + next)
 
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
│ padding if required           │ ← aligns payload start addr
├───────────────────────────────┤
│       PAYLOAD                 │ ← returned to user
├───────────────────────────────┤
│  Slop (maybe remains at end)  │ ← aligns block size if payload size requested != multiple of 16
└───────────────────────────────┘
</pre>

# free() doesnt do any checks on the pointer given

The C standard doesnt require free() to do any checks beyond what the programmer guarentees.<br>
**Extra checks cost performance** as validation of ptr is pretty hard as many invariants:

- `ptr` could point inside a payload not the block start.
- `ptr` could be from a different allocator or memory region.
- `ptr` could be already freed.
- Reading headers blindly could crash or corrupt memory if `ptr` is completely invalid.

So, if ptr given to free() is not NULL or valid(not yet freed allocation), the behaviour is undefined. 

- It may segfault if it tries to access virtual mem addresses that aren't mapped.

- If it receives a pointer to mapped but invalid memory, the allocator is allowed to do anything, including corrupting itself.

**In case of returning mem to OS (`munmap()`):**

The allocator still would have to do some local checks on the metadata before it returns virtual memory to the OS.

# `sbrk()`/`brk()` vs `mmap()`

**What `sbrk()` does:**

Moves the program break- the end of the process's heap

- Grows or shrinks the heap contiguously

- There is exactly one heap.

**Problems:**

1\. Memory can only grow "upward" as one big chunk.

2\. Hard or impossible to return memory in the middle back to the OS.

3\. Interferes badly with shared libraries and other allocators.

4\. Not thread-friendly.

**What `mmap()` does:**

Allocates a new page in the process's virtual address space anywhere where unmapped areas are available.

- Fragments the heap(many independent regions).

- Any memory chunk mapped by `mmap()` can be returned to the OS(more flexibility and works well with cache system).

**If mmap() and sbrk() are used together:**

A\. The kernel will not let `sbrk()` grow the heap into an existing `mmap()` region. sbrk() simply fails.

You lose address space flexibility. The OS chooses where mmap() regions go.

If you try later:
<pre>sbrk(large_amount) </pre>
The kernel might say no virtual space left above the heap even though there is plenty of virtual memory left elsewhere.

So sbrk() fails but mmap() would have succeeded.

B\. Two unrelated memory sources. Your allocator now has:

- Memory that can be returned (`mmap()`)

- Memory that mostly cant be returned (`sbrk()`)

This makes book-keeping harder.





