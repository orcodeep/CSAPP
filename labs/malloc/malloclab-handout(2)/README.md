_This is the handout given in 2017 for malloclab by utah uni_

**Getting Started**

Start by unpacking malloclab-handout.zip. The only file you will be modifying and handing in is "mm.c". The "mdriver.c" program is a driver program that allows you to evaluate the performance of your solution. Use make to generate the driver code and run it as

<pre>$ ./mdriver -t &lt;trace file/folder&gt; </pre>

See Trace-based Driver Program for information about command-line flags to mdriver.

When you have completed the lab, you will hand in only one file, "mm.c", which contains your solution.

**How to Work on the Lab**

Your dynamic storage allocator will consist of the following five functions, which are declared in "mm.h" and defined in "mm.c":
<pre>

  int   mm_init(void);

  void *mm_malloc(size_t size);

  void  mm_free(void *ptr);

  

  int   mm_check(void);

  int   mm_can_free(void *ptr);
</pre>

The "mm.c" file that we have given you implements the simplest but still functionally correct malloc implementation that we could think of, except that it does not properly implement mm_can_free. Using this as a starting place, modify these functions (and possibly define other private, static functions), so that they obey the following semantics:

1\. mm_init: Before calling mm_malloc or mm_free, the application program (i.e., the trace-driven driver program that you will use to evaluate your implementation) calls mm_init to perform any necessary initialization, such as allocating the initial heap area. The return value should be -1 if there was a problem in performing the initialization, 0 otherwise.

The mm_init function will be called once per benchmark run, so it can be called multiple times in the same run of mdriver. Your mm_init function should reset your implementation to its initial state in each case.

2\. mm_malloc: The mm_malloc function returns a pointer to an allocated block payload of at least size bytes, where size is less than 232. The entire allocated block should lie within the heap region and should not overlap with any other allocated block.

    - We’ll compare your implementation to the version of malloc supplied in the standard C library (libc). Since the libc malloc always returns payload pointers that are aligned to 16 bytes, your malloc implementation should do likewise and always return 16-byte aligned pointers.

3\. mm_free: The mm_free function frees the block pointed to by ptr. It returns nothing.

This routine is only required to work when mm_can_free(ptr) returns 1. In particular, it will always work for a correctly behaved client that provides a ptr returned by an earlier call to mm_malloc and not yet freed via mm_free.

4\. mm_can_free: The mm_can_free function takes a pointer and returns 1 if the pointer would be a valid argument to mm_free, and it returns 0 if the pointer is an invalid argument to mm_free. A valid pointer is one that was returned by a previous call to mm_malloc and not yet passed to mm_free.

If the client program is misbehaved and corrupts the heap, then mm_can_free is allowed to return 0 even if the given pointer was from a previous call to mm_malloc and not yet passed to mm_free. More generally, if mm_check returns 1 and mm_can_free returns 1 for a given pointer, then mm_free on the pointer must not crash, and it must leave the allocator in a state such that mm_check returns 1.

5\. mm_check: The mm_check function checks whether the heap is well-formed so that calls to mm_malloc and mm_free will behave correctly and not cause a crash, in which case it returns 1. The mm_check must return 0 if the heap has been corrupted by the client so that mm_malloc and mm_free cannot behave correctly or might crash.

- The intent is that a client can use mm_check to help debug their own code—but you will likely find that mm_check is useful for debugging the allocator, too. We will test mm_check by calling it multiple times during a well-behaved client to make sure that it always returns 1. We will also test mm_check with a chaotic client that randomly and incorrectly writes to memory that is outside allocated regions returned by mm_malloc; in that case, mm_check might return 0 to indicate that the heap is too corrupted to continue, or it may return 1 as long as mm_malloc and mm_free will still not crash—at least when mm_free is only called on pointers for which mm_can_free returns 1.

- The mm_check function is not obligated to detect arbitrary misbehavior by a client. For example, if a client program mangles the heap in a way that turns out to be a different but consistent state as far as the allocator is concerned, then mm_check can return 1. The mm_check function is only obligated to detect heap changes that prevent the allocator’s other functions from working without crashing.

Beyond correctness, your goal is to produce an allocator that performs well in time and space. That is, the mm_malloc and mm_free functions should work as quickly as possible, and the total amount of memory used by your allocator should stay as close as possible to the amount of memory needed to hold the payload of mm_malloc calls not yet balanced by mm_free calls.

**Support Functions**

The "memlib.c" module provides a wrapper on the operating system’s virtual-memory system. Your allocator will need to use these functions to obtain pages of memory that it can allocate from. When mdriver runs your allocator, it resets the memory system (i.e., frees all pages) before calling mm_init to start a new benchmark run.

You can invoke the following functions from memlib.c:

- size_t mem_pagesize(void);

    - Returns the minimum granularity of allocation via mem_map and deallocation via mem_unmap.

    - You can assume that the allocation granularity is a power of 2 and at least 4096.

- void *mem_map(size_t amt);

    - Returns a pointer to the start of group of newly allocated, contiguous pages that occupy exactly amt bytes.

    - The given amt must be a multiple of the value produced by mem_pagesize, and the returned address is always a multiple of the value produced by mem_pagesize. Separate calls to mem_map may not return pages that are contiguous to pages from previous calls.

    - The result is NULL if memory cannot be allocated.

- void mem_unmap(void *addr, size_t amt);

    - Releases back to the operating system a group of contiguous pages that start at addr and cover exactly amt bytes.

    - The given addr must be page-aligned (i.e., it must be a multiple of the value produced by mem_pagesize), and amt must be a multiple of the value produced by mem_pagesize. The given addr must also identify pages formerly allocated with mem_map and since released by mem_unmap. A single mem_unmap can release pages allocated by multiple previous mem_map calls, and multiple different mem_unmap calls can release pages allocated by a single mem_map call—as long as each individual page’s allocation is balanced by exactly one deallocation.

- size_t mem_heapsize(void);

    - Returns the total size of all currently allocated pages (i.e., mapped and not yet freed).

- int mem_is_mapped(void *addr, size_t amt);

    - Returns 1 if all pages spanning addr (inclusive) through addr+amt (not inclusive) are mapped via mem_map, 0 otherwise.

    - The given addr must be page-aligned (i.e., it must be a multiple of the value produced by mem_pagesize), and amt must be a multiple of the value produced by mem_pagesize.

While your allocator will obviously need to call mem_map to obtain memory for allocation, you can improve space-efficiency for this assignment using mem_unmap to avoid retaining pages that are not needed as allocated blocks are freed.

The mem_is_mapped function is useful for implementing mm_check and mm_can_free, since those functions may need to check whether an arbitrary address can be safely read.

You can assume that your allocator is the only part of a program that calls mem_map and mem_unmap, except that all pages are reset before mm_init is called.

**Trace-based Driver Program**

The driver program "mdriver.c" tests your "mm.c" implementation for correctness, defensiveness, space utilization, and throughput:

- The correctness pass runs a well-behaved sequence of mm_malloc and mm_free calls. It also includes a mm_check call after every step to make sure that the result is always 1. It calls mm_check_free before every call to mm_free to make sure that mm_check_free returns 1. After a sequence of mm_free calls, it uses mm_can_free to make sure the result is 0 for each just-freed pointer.

- Any failures in this pass correspond to bugs in the allocator. Specifically, if mm_check ever returns 0, then that’s a bug in the allocator (possibly in the heap checker), and not in the client. Also, if mm_can_free returns 1 for an already-freed pointer or if it returns 0 for a pointer that should be freeable, then that’s a problem with the allocator/checker.

- This pass can take a while, depending on how long mm_check and mm_can_free take, but that’s ok, as long as it’s on the order of seconds instead of minutes.

- The defensiveness pass runs the same trace, but with random writes (potentially and likely invalid) to mapped pages, stopping when mm_check returns 0 and only calling mm_free when an immediately preceding mm_can_free returns 1.

- This pass can be repeated multiple times, if requested. It will likely run quickly, because random chaos normally provokes a 0 result from mm_check fairly soon. If there’s a problem with mm_check or mm_can_free, this pass will most likely crash with a segmentation fault (which counts as an incorrect allocator implementation).

- The space utilization pass runs the trace again, this time without mm_check and mm_can_free calls. It measures the amount of memory that is occupied by mapped pages, so that the memory use can be compared to the minimum memory use possible to satisfy mm_malloc requests.

- The throughput pass runs the trace one last time without mm_check and mm_can_free calls, this time measuring the speed of allocation and deallocation.

- To perform well in this pass, your mm_malloc and mm_free implementation should not include defensive checks. The client will be well-behaved for performance tests.

The driver program is controlled by a set of trace files that are included in the malloclab-handout.zip archive. Each trace file contains a sequence of allocate, free, and reallocate (i.e., allocate plus free) directions that instruct the driver to call your mm_malloc and mm_free functions in some sequence. The driver and the trace files are the same ones we will use when we grade your handin "mm.c" file, although we may change the order in which the traces are used.

The provided makefile combines "mdriver.c" with your "mm.c" to create mdriver, which accepts the following command line arguments:

- -t ‹tracedir› — Look for the default trace files in directory ‹tracedir› instead of the default directory that is defined in config.h.

- -f ‹tracefile› — Use one particular ‹tracefile› for testing instead of the default set of tracefiles.

- -n — Skip all calls to mm_check and mm_can_free when testing correctness, and skip defensiveness tests. This flag is useful for more quickly getting performance results for an allocator that works. Also, the initial provided implementation will run with defensiveness checks disabled.

- -r ‹repeats› — Set the number times that the defensiveness check runs for each trace, where each run’s client misbehaves in a different random way.

- -s ‹seed› — Set the random-number generator’s seed for random chaos during defensiveness checking. By default, chaos uses a fixed seed, so that you can see the same results every time.

- -l — Run and measure libc malloc in addition to the "mm.c" malloc implementation.

- -q — “Quiet” mode, showing only the final performance score after all correctness and defensiveness tests pass.

- -h — Print a summary of the command line arguments.

We will test your program using a flag like -r 1000 to try many different instances of random chaos for the defensiveness pass.

You may find it useful to see the shape of memory use created by a trace file. Run

<pre>$ racket plot.rkt ‹tracefile›</pre>

to see a plot of allocated memory over time for ‹tracefile›.<br>
**Programming Rules**

- Your "mm.c" must be implemented in ANSI standard C, as always.

- You must not change any of the interfaces in "mm.c". We will compile your "mm.c" with a fresh copy of the driver files.

- You must not invoke any memory-management related library calls or system calls, including mmap, malloc, calloc, free, realloc, sbrk, brk or any variants.

- You must not define any global or static compound data structures such as arrays, structs, trees, or lists in your "mm.c" program. However, you are allowed to declare types (including struct types) in "mm.c", and you are allowed to declare global scalar variables such as integers, floats, and pointers in "mm.c".

- For consistency with the libc malloc implementation, which returns blocks aligned on 16-byte boundaries, your allocator must always return pointers that are aligned to 16-byte boundaries. The driver will enforce this requirement for you.

**Evaluation**

Your grade will be calculated as follows:

- Correctness + defensiveness — Your performance grade will be scaled by the percentage of traces that your allocator handles correctly for both the correctness and defensiveness passes.

Performance — Three performance metrics will be used to evaluate your solution:

- Space utilization: The ratio between the peak aggregate amount of memory used by the driver (i.e., allocated via mm_malloc but not yet freed via mm_free) and the peak total size of pages used by your allocator. The optimal ratio is 1. You should find good policies to minimize fragmentation in order to make this ratio as close as possible to the optimal.

- Instantaneous space utilization: The geometric mean of the ratio between the amount of memory used by the driver at after each step of the trace and and the total size of pages used by your allocator at that point. The optimal ratio is 1. Since the amount of memory allocated by a trace goes goes down as well as up during the trace, an optional solution will release memory pages as they are no longer needed. At the same time, beware that constantly mapping and unmapping pages can take too much time.

- Throughput: The average number of operations completed per second.

The driver program summarizes the performance of your allocator by computing a performance index, P, which is a weighted sum of space utilization U, instantaneous space utilization Ui, and throughput T relative to a baseline through Tlibc:

`P = 0.3U + 0.3Ui + 0.4T/Tlibc`

The value of Tlibc is 9000K ops/second. We will run your code on a CADE machine, as usual, and we’ll also supply the -l flag to warm up the processor and cache before measuring your functions.

Observing that both memory and CPU cycles are expensive system resources, we adopt this formula to encourage balanced optimization of both memory utilization and throughput. Since each metric will contribute at most a fraction to the performance index, you should not go to extremes to optimize either the memory utilization or the throughput only. To receive a good score, you must achieve a balance between utilization and throughput. Note that the performance index overall favors space utilization over throughput.

The ideal performance index of 100 is unreachable, since every allocator will have some overhead that reduces utilization. A coalescing allocator with an implicit free list should reach around 44 easily. A coalescing allocator with an explicit free list can reach 50 to 60 fairly easily. We’ll use the following thresholds to assign grades:

-    0 to 44: linear scale from 0% to 80%
-    44 to 50: linear scale from 80% to 100%
-    50 to 60: 100%
-    60 to 80: linear scale from 100% to 110%.
-    80 to 100: linear scale from 110% to 115%.

As a secondary constraint, your allocator must be able to perform the following sequence within 5 minutes: pass the correctness tests for all traces plus the defensiveness test on "traces/amptjp-bal.rep".

**Heap-Check Tips**

Dynamic memory allocators are notoriously tricky beasts to program correctly and efficiently. They are difficult to program correctly, because they involve a lot of untyped pointer manipulation. Even if the assignment did not require mm_check and mm_can_free, you would find it very helpful to write a heap checker that scans the heap and checks it for consistency.

Some examples of what a heap checker might check are:

- Are block headers and footers consistent?
- Is every block in the free list marked as free?
- Is every free block actually in the free list?
- Do the pointers in the free list point to valid free blocks?
- Do any allocated blocks overlap?
- Do the pointers in a heap block point to valid heap addresses?
- Are there any contiguous free blocks that somehow escaped coalescing?

Since a misbehaved client can corrupt memory pages used by the allocator in arbitrary ways, you cannot assume that pointers set up by your allocator will remain valid when mm_check is called. Before dereferencing a pointer, make sure that the dereference will succeed by using mem_is_mapped. Since mem_is_mapped works at the level of pages, you’ll probably find it best to implement a ptr_is_mapped helper function that takes an address and size and makes sure that the address range is on mapped pages:

<pre>
  /* rounds up to the nearest multiple of mem_pagesize() */

  #define PAGE_ALIGN(sz) (((sz) + (mem_pagesize()-1)) & ~(mem_pagesize()-1))

  

  /* rounds down to the nearest multiple of mem_pagesize() */

  #define ADDRESS_PAGE_START(p) ((void *)(((size_t)p) & ~(mem_pagesize()-1)))

  

  int ptr_is_mapped(void *p, size_t len) {

    void *s = ADDRESS_PAGE_START(p);

    return mem_is_mapped(s, PAGE_ALIGN((p + len) - s));

  }
</pre>

A heap checker that scans all allocated pages once in linear time will be fast enough. Beware that a heap checker that is somehow quadratic in the size of allocated pages will be impractically slow. A heap checker that attempts to take a snapshot of the heap state and compare it later is unlikely to work, since the snapshot data will have to be recorded in an allocated page, and a chaotic client can modify the snapshot, too.

Random chaos to check your implementation’s defensiveness will modify only mapped pages, and not your allocator’s global variables—but that doesn’t help much, since the programming rules constrain your allocator’s use of global variables.

**More Tips**

- Start early! It is possible to write an efficient malloc package with a few pages of code. However, an allocator could easily be the most difficult and sophisticated code you have written so far.

- Use the mdriver -f option. During initial development, using tiny trace files will simplify debugging and testing. We have included two such trace files, "short1-bal.rep" and "short2-bal.rep", that you can use for initial debugging.

- Use the mdriver -l options to warm up the processor and cache before running your functions.

- Compile with gcc -g and use a debugger. A debugger will help you isolate and identify out of bounds memory references.

- Understand every line of the malloc implementations in the textbook and slides. The textbook and slides both have a detailed example of a simple allocator based on an implicit free list. Use them as a point of departure. Don’t start working on your allocator until you understand everything about the implicit-list allocator. That simple allocator will not work as-is for this assignment, however, since it’s based on sbrk instead of mem_map.

- Encapsulate your pointer arithmetic in functions or C preprocessor macros. Pointer arithmetic in memory managers is confusing and error-prone because of all the casting that is necessary. You can reduce the complexity significantly by writing macros for your pointer operations. See the textbook and slides for examples.

- If you use macros, put each use of a macro argument in parentheses within the macro definition, and always put parentheses around the right-hand side of a macro definition. Otherwise, it’s easy to write macros that parse differently than you expect when the macro is textually expanded.

- Complete your implementation in stages. Get a basic implementation working, and then modify it to improve performance.

- Use a profiler. You may find the gprof tool helpful for optimizing performance.

- Keep in mind that mmap (especially) takes time, so you’ll want to make as few calls to mem_map as possible while achieving good utilization.

