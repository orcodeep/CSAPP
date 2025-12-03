# No hit situation:

You scan the set for a hit, but none of the lines match (cache miss).

You do not care about evictions if there is a hit but u do care about the `edgeCounter` if there is a hit because the direction of aging might have changed.

### Why flipping the direction matters:

Suppose all lines have reached the extreme age (say 255).

If there is a miss then:

- From the eviction perspective, it doesn’t matter which line you evict — all lines have the same age. 

The purpose of flipping the increasing flag here is not for eviction, but for the next access:

- The next hit will age a line in the correct direction (opposite to the previous extreme).

- That ensures the cyclic LRU pattern continues correctly.

- After that, future misses will now have meaningful age differences to pick the “oldest” line.

If you didn’t flip the direction, your logic for picking the “oldest” line (for eviction) for next misses after a hit would incorrectly assume ages are still increasing.

## TL;DR:

- Flipping the increasing flag on “all extreme” is about future aging, not about immediate eviction.

- The first miss when all ages are extreme is “neutral” for eviction.

- The flip ensures the cyclic aging direction is correct for subsequent accesses, which preserves the intended LRU behavior.

# Returning array 

If `arr` is allocated on the stack inside `parse()`.

`int arr[3] = {hits, misses, evictions};`

- When the function returns, the memory for `arr` is freed automatically.

- int* arr = parse(...); in `main()` will point to invalid memory → undefined behavior.

