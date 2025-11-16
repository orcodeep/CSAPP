x/`<N><format> <address>`
x → examine memory

`<N>` → how many units to show (here, 8)

`<format>` → how to interpret each unit (here, d = decimal)

`<address>` → where to start (here, $rsp)

(gdb) x/8d $rsp <br>
Memory (x): you are asking GDB to look directly at memory, not registers.

$rsp → starting at the current stack pointer

8d → print 8 decimal integers in sequence from that address  

Size of each unit:

d means signed 4-byte (32-bit) integer by default.  
So x/8d shows 8 × 4 bytes = 32 bytes in total.