The lab guarantees:

- All accesses are aligned

- No access crosses a cache block boundary

- Each access fits perfectly inside one block

So the “size” does not affect which block is touched, nor how many blocks are touched.

so we only need:

1. operation (L/S/M)
2. address

We ignore `size`

so for the Cache Lab, even if the block size were 2 bytes (ridiculously tiny), you can still assume a double fits inside a single block, because the lab explicitly guarantees that no memory access ever crosses a cache block boundary


