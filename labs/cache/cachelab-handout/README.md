# No hit situation:

You scan the set for a hit, but none of the lines match (cache miss).

Before inserting a new line or choosing a victim, you need to know the correct aging direction to pick the line that is “oldest” according to the cyclic LRU policy

You do not care about evictions if there is a hit but u do care about the `edgeCounter` if there is a hit because the direction of aging might have changed.

### Why flipping the direction matters:

Suppose all lines have reached the extreme age (say 255) and increasing = 1.

If you didn’t flip the direction, your logic for picking the “oldest” line (for eviction) would incorrectly assume ages are still increasing.

By flipping increasing before eviction, you ensure:

- The next age update will happen in the correct direction (decrement, in this example).

- Your eviction selection (min/max age depending on increasing) now chooses the correct line according to cyclic LRU.

