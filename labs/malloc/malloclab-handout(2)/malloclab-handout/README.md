We will have powers of 2 divisions of lists. 

The index into the freelist array = 31 - __builtin_clz(size)

Points:- 

1\. Since allocated blocks dont have footer, the footer space can be used for payload.

2\. If the user requests a payload(p) where `p % 16 != 0`, we will pad the end to make the whole block 16 byte aligned. So freelists will contain blocks which are multiples of 16.

- Hence if you have a large block and you want to split it into two pieces- both pieces must be 16 byte aligned and of size > min block size (usually `32` bytes).




