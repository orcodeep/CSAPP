Remember to run the `targets` with a `-q` flag. so that they don't try to contact a non-existent grading server.

# hex2raw may append a newline(0a)

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

# In level1:

Inside getbuf disassembly we see `sub $0x28,%rsp`. 0x28 = 40. Although the function reserves 40 bytes of stack space, in this particular function that entire region is the buffer, because there are no other local variables. So the 40 bytes end up being used entirely as the buffer.

Hence for upto a string of 39 chars getbuf returns 1. (39chars + 1 null terminator).


