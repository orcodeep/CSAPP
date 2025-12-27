
[Implementation details](#implementation)

# Given in mm.c

## What PAGE_ALIGN() macro does

PAGE_ALIGN “rounds up” your requested size to a whole number of pages.

Suppose `mem_pagesize()` = 4096 bytes.<br>
Your requested size is 5000.

Now apply PAGE_ALIGN(size):
- PAGE_ALIGN(5000)=(5000+4096−1)&∼(4096−1)

Step by step:

- 5000 + 4096 - 1 = 9095
- ~(4096 - 1) = ~4095 = 0xFFFFF000 (in hex, masks lower 12 bits)
- 9095 & ~4095 = 8192<br><br>
_8192 is 4096*2._

So PAGE_ALIGN(5000) = 8192. 

**Interpretation**:

1\) Instead of requesting 5000 bytes from the OS, you request 8192 bytes, which is a multiple of the page size.

2\) This ensures the heap is allocated in full pages.

### Important:

**`mmap()` always returns pointers(ptr to start of allocated pages) which are multiples of 16.**
Since:
- Strict Page Alignment: `mmap` is required by the OS kernel to return an address that is aligned to the start of a page.
    - mmap() returns a pointer that is a multiple of the system page size, e.g., 0x1000, 0x2000, 0x3000…

    - Since 4096 is divisible by 16, these addresses are also multiples of 16, so they satisfy 16-byte alignment automatically.

**Hence if our freelist blocks are all multiples of 16, padding inside each block before the payload starts is constant.**

Why this is good:
- **IMP-** **padding not being variable means when you `free()` header lookup is O(1).**
- Payload alignment then costs zero padding(or doesnt require any kind of arithmetic).
- splitting and coalescing never drift.

## What ALIGN() macro does

It makes the user requested size 16 byte aligned

- Since we do not need to work for aligning the payload start addr. Our only worry is aligning the paylaod size.

- \**We should align the `size` user requests before we start to find a suitable block for it in freelist.

## current_avail & current_avail_size

<pre>
| block A (allocated) | block B (free) | block C (allocated) | ... | current_avail region |
</pre>

`current_avail` can be though of as the first byte of heap memory beyond all existing allocated or free blocks.
- It does not traverse the heap; it just grows forward as you allocate more from it.
- This design separates recycling old memory (freelist) from requesting fresh memory (current_avail).

`current_avail_size` is the amount of unused memory left in the current fresh chunk that `current_avail` points to.
<pre>
current_avail -> |   free space in chunk   |
current_avail_size = number of bytes in that free space
</pre>

_Why is `current_avail_size` stored as an int?_<br>
It’s still used as `int` in many simple allocators:

- These allocators assume malloc requests < 2 GB, which is reasonable for many applications.<br><br>
The handout specifies that `mm_malloc()` func returns a ptr to an allocated block payload of at least size bytes, where size is less than `2`<sup>**32**</sup>.

- Using `int` saves space in the allocator’s global data.

# Implementation

[seg freelist implementation](#freelist)

## Heap start and end

**Prologue allocation**

When you initialize the heap in `mm_init()`, you create a prologue block:

- Header: size = minimal block size (e.g., 16 bytes), allocated = 1
- Optional footer: doesn’t matter, can be 0

You then increment current_avail past the prologue block so that allocations start after the prologue.<br>
And you also set the `if prev allocated` flag in the block that current_avail ptr points to.
- So that when a new block is allocated it already knows if its predecessor block is allocated or free.
- This ensures that when the first free block appears, it will never try to coalesce backward into the prologue.

You also mark the current addr of current_avail as `heapStart` global.
- `heapEnd` is always just the current_avail ptr.
- `mm_check()` will traverse the blocks between `heapStart` and `current_avail`.

**Nothing “special” in the prologue itself**

The prologue is just a normal allocated block; it doesn’t store a pointer or magic value.

It’s not meant to be freed, but you don’t need to protect it:
- You assume the allocator code never frees the prologue.
- The programmer can’t access it anyway, because allocations start after it.


## freelist

Our segregated freelist implementation:

|  Range   |Stratergy |  Search in list | Indexing | Why?  |
|:--------:|:--------:|:---------------:|:--------:|:------|
|64B-1024B |Linear 64B steps|First-Fit  |Direct Indexing<br>i = (ALIGN(size) - 1) / 64|Many fine-grained lists:<br>1\. Very low internal Fragmentation<br>2\. Fast allocation|
|1024B-1MB |Power of 2      |First-Fit  |For loop on table containing size classes OR clz(Math) |10 bins(2<sup>10</sup> to 2<sup>20</sup>) efficiently handles medium buffers|
|1MB-4MB   |single list|Best-Fit|index = 26|Request for such large blocks is uncommon so due to less population of such blocks we can be thorough|
|4MB-8MB   |single list|Best-Fit|index = 27|&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;&nbsp;"|
|>8MB      |Direct OS call|Nil|Nil|No need to cache such big blocks. When user is done with the block we return it to the OS|


Some points:- 

1\. Since allocated blocks dont have footer, the footer space can be used for payload.

2\. If the user requests a payload(p) where `p % 16 != 0`, we will pad the end to make the whole block 16 byte aligned. So freelists will contain blocks which are multiples of 16.

- Hence if you have a large block and you want to split it into two pieces- both pieces must be 16 byte aligned and of size > min block size (usually `48` bytes on 64bit systems).<br><br>
Header: 8 bytes<br>
Prev pointer: 8 bytes<br>
Next pointer: 8 bytes<br>
Padding: 8 bytes (to make payload start 16-byte aligned)<br>
Footer: 8 bytes<br><br>
So far: 8 + 8 + 8 + 8 + 8 = 40 bytes<br>
To satisfy 16-byte alignment for the block size itself, you round up to 48 bytes. which means more 8 byte padding before footer.<br><br>
Hence, `min block size` = `48` bytes and this supports a `paylaod` of `16` bytes max **but we will have 64 byte min paylaod**.

3\. **Splitting a block may need Re-categorization of split free block into a different list.**

### How we allocate.

To minimize fragmentation and avoid wasting memory, the allocator should:

1\) Search the entire sgregated freelist first. Searching higher lists 1 by 1 if suitable block couldn't be found in the list you indexed into. 
- Look for a free block that satisfies the request(including splitting if applicable).

2\) Only allocate from `current_avail` region is no suitable free block exists.
- This grows the heap only when reuse is impossible. 




