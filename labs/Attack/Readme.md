Remember to run the `targets` with a `-q` flag. so that they don't try to contact a non-existent grading server.

# Text editors automatically append a newline 
It may be found that the actual file ends with 0a. after `./hex2raw -i level1.txt > level1.raw` even if no '\n' was entered in the txt file.

This can be confirmed with :- `xxd level1.raw`

output:-<br>
00000000: 4545 4545 4545 4545 4545 4545 4545 4545  EEEEEEEEEEEEEEEE<br>
00000010: 4545 4545 4545 4545 4545 4545 4545 4545  EEEEEEEEEEEEEEEE<br>
00000020: 4545 4545 4545 4545 c017 4000 0000 0000  EEEEEEEE..@.....<br>
00000030: `0a`<br>

## Prevention:

Do: `truncate -s -1 level1.raw`

Now `xxd level1.raw` should give:<br>
<pre>
00000000: 4545 4545 4545 4545 4545 4545 4545 4545  EEEEEEEEEEEEEEEE
00000010: 4545 4545 4545 4545 4545 4545 4545 4545  EEEEEEEEEEEEEEEE
00000020: 4545 4545 4545 4545 c017 4000 0000 0000  EEEEEEEE..@.....
</pre>

Now `./ctarget -q < level1_raw.txt` passes.