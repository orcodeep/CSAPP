Remember to run the `targets` with a `-q` flag. so that they don't try to contact a non-existent grading server.

# hex2raw may append a newline(`0x0a`)

It may be found that the actual file ends with 0a. after `./hex2raw -i level1.txt > level1.raw` even if no '\n' was entered in the txt file.

This can be confirmed with :- `xxd level1.raw`
<pre>
$ xxd level1.raw
00000000: 4545 4545 4545 4545 4545 4545 4545 4545  EEEEEEEEEEEEEEEE
00000010: 4545 4545 4545 4545 4545 4545 4545 4545  EEEEEEEEEEEEEEEE
00000020: 4545 4545 4545 4545 c017 4000 0000 0000  EEEEEEEE..@.....
00000030: 0a
</pre>
"00000030: `0a`"

And also with `wc -c level1.raw`
<pre>
$ wc -c level1.raw
49 level1.raw 
</pre>

## Prevention:

Do: `truncate -s -1 level1.raw`

Now `xxd level1.raw` should give:<br>
<pre>
00000000: 4545 4545 4545 4545 4545 4545 4545 4545  EEEEEEEEEEEEEEEE
00000010: 4545 4545 4545 4545 4545 4545 4545 4545  EEEEEEEEEEEEEEEE
00000020: 4545 4545 4545 4545 c017 4000 0000 0000  EEEEEEEE..@.....
</pre>
And `wc -c level1.raw`
<pre>48 level1.raw</pre>

Now `./ctarget -q < level1.raw` passes.

# In phase1:

Inside getbuf disassembly we see `sub $0x28,%rsp`. 0x28 = 40. Although the function reserves 40 bytes of stack space, in this particular function that entire region is the buffer, because there are no other local variables. 

So, the 40 bytes end up being used entirely as the buffer.
Hence for upto a string of 39 chars getbuf returns 1. (39chars + 1 null terminator).

Important:
- buf[0] is NOT the highest address.
- buf[0] is the LOWEST address in the buffer.

Because the stack grows downward, the compiler allocates the buffer like this:
<pre>
HIGH ADDR    → buf[n-1]
                buf[1]
                ...
LOW ADDR     → buf[0]
</pre>

Gets writes sequentially from buf[0] → buf[1] → … → buf[n-1]
(low → high addresses):

- That means it writes into lower addresses first (bottom of the buffer), then upward towards buf[n-1].

- When you overflow past the buffer, you continue writing into even higher addresses

- And those higher addresses contain saved RIP

<pre>
Higher addresses
+---------------------------+
| Return address (saved RIP)|  <-- still here
+---------------------------+
| Saved RBP (if any)        |
+---------------------------+
| buffer[39]                |
| ...                       |
| buffer[0]  <-- new %rsp   |
+---------------------------+
Lower addresses
</pre>

# In phase2:

We overflow the buffer and overwrite the address on the stack where it normally contains an address inside `.text` which the `rip` goes to after popping that stack adddress due to a `ret` in `.text`. We overwrite the `.text` address inside that stack address with the address of `buf[0]` which is a stack address.

- And because for this level the stack is executable, if we put machine instructions inside the buffer, they will be executed when pointed at by `rip`.

The address of `touch2` should be pushed on the stack before calling it. So we can call it with the `ret` instruction. 

- Because when first ret is hit from getbuf, `addr of buf[0]` is popped which was at ret addr slot i.e When ret executes, it consumes 8 bytes at `rsp` and then increments RSP by 8. <br><br>So, now `rsp` points at an address which contains stuff thats not our function's (caller's saved registers, caller's local registers etc etc).

<pre>
Higher addresses
+---------------------------+
| [ caller’s saved frame ]  |<-%rsp here (← previous function’s stuff) 
+---------------------------+--+
|  address of buf[0]        |  |
+---------------------------+  |
| buffer[39]                |  | --> all this is deallocated but they are
| ...                       |  |     not erased
| buffer[0]  <-- new %rip   |  |
+---------------------------+--+
Lower addresses
</pre>

From inside the buffer when we `pushq` the address of `touch2`, it gets stored at the ret address slot where previously `address of buf[0]` was. So now when we `ret` from inside the buffer the address of touch2 gets popped automatically from the stack for rip to point to.
<pre>
Higher addresses
+---------------------------+
| [ caller’s saved frame ]  |<- %rsp here, ← previous function’s stuff  
+---------------------------+
|  address of touch2        |<- %rsp here after pushq from buffer
+--------------------------------+--+
| buffer[39]                     |  | 
| ...                            |  |
| buffer[xx] = pushq addr[touch2]|  | --> all this was deallocated but they
| ...                            |  |     are not erased
| buffer[0]  <-- new %rip        |  |
+--------------------------------+--+
Lower addresses
</pre>

As always `gdb` can be used to find out values of `rsp` and other registers at a specific time.

### padding:

The standard padding is just repeating 0x90 (`nop`).

When the CPU sees a NOP, it just says:

- Okay nothing to do and moves the `rip` to the next byte.

**If instead of `push` we overwrite an address higher than ret addr slot of getbuf() by `8` without aligning the stack we would `segfault`.<br><br>We would enter the function touch2 but as soon as touch2 sees that rsp doesnt end in a 8 it would segfault.**


## ***Alternate solution:***

Write both the addresses of buf[0] and touch2 to stack with buffer overflow 

But it is important where the Stack Pointer `%rsp` ends up after a pop. 

After ret executes and rip points at touch2, %rsp must end in 8.

- This is the "Safe State" for the function to run.

- The function expects the stack to be "misaligned" by 8 bytes (because a normal call instruction would have pushed an 8-byte return address).

In gdb:

- Before the first ret from getbuf(), `rsp` = `0x5561dca0`

- After first pop due to ret in getbuf() the `rsp` increments by 8, `rsp` = `0x5561dca8` and because the last digit is `8` it is in required state.

- Then when `rip` executes `ret` from inside the buffer, after execution again `rsp` = `rsp+0x8` so `rsp`=`0x5561dcb0` which is unsafe state and this will cauase `segfault1`.

### **The Solution:- The `"ROP NOP"`**

To shift the stack pointer by 8 bytes without changing anything else, you simply include the address of a `ret` instruction in your chain.

That's it. You find the address of a ret instruction anywhere in the program and you place that address in your stack chain.

**The ret instruction is "dumb." It does not care about alignment. The ret gadget is then used to "absorb" the bad alignment**

A ret instruction does two things:

- Pops the next address off the stack.

- Jumps to it.

If you jump to a ret instruction, it effectively says "skip this slot and go to the next one." It acts exactly like a `nop` (No-Operation) in code injection. For ROP chains, it consumes 8 bytes of stack space, toggling the alignment.

**The popq instruction is just like the ret instruction: It does not care about alignment.<br>Just like ret, popq is a "Goat." It eats anything**

So we could write the address of the `ret` inside the buffer.

- After the `ret` from buffer is executed it pops the next 8 bytes from the rsp(rsp = `0x5561dca8` right now).<br><br>And if the value in this address is the address of that same `ret` instruction in the buffer the `rip` essentialy doesnt move but the `rsp` gets icremented by `8` as normal `rsp+0x8`=`0x5561dcb0` but even though the end is `0` its safe as `rip` points at a `ret`.

- Now at `0x5561dcb0` if we have the address of `touch2`. when `ret` is again executed by `rip` and rip is at beginning of `touch2`, rsp gets incremented to `0x5561dcb8` which is a safe state as it ends in `8`.

__This will be used extensively for `ROP` becasue we have only the high stack addresses to write the addresses of all the gadgets and to chain them together.__

# In phase3:

We cant store the null terminated string in the buffer because hexmatch and strcmp will overwrite it. so store the null terminated string in the stack.

**Also if you push to the stack from inside the buffer, as the stack grows the buffer gets overwriten.**

So, if there is machine instructions near the end of the buffer, the get overwritten and `rip` will execute those and program would `segfault`.

Hence, instructions should finish before the bytes that get overwritten because of the push instructions in the buffer.

