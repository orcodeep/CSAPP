# In phase_2:

    0x0000000000400f25 <+41>:    add    $0x4,%rbx
    0x0000000000400f29 <+45>:    cmp    %rbp,%rbx
    0x0000000000400f2c <+48>:    jne    0x400f17 <phase_2+27>
    0x0000000000400f2e <+50>:    jmp    0x400f3c <phase_2+64> 


This makes sure all the elements of the array are checked before jumping to phase_2 + 64
<pre>
%rbp has address of the end of array and  

%rbx has address of the curretn element of array

cmp %rbp,%rbx sets ZF=1 if addr[rbx]==addr[rbp]
when ZF=1, jne=false => jmp will be excuted
</pre>

Note:

The `stack` grows `downward`

So when we say increment the stack we mean `shrink` the stack<br>
`add $0xXX, %rsp` instruction shrinks the stack<br>
larger address means up the stack i.e mentally down the stack if stack grows upward

when we say decrement the stack we mean `grow` the stack<br>
`sub $0xXX,%rsp` grows the stack

# In phase_3:
<pre>
sub $0x18, %rsp         ; allocate space for local variables
lea 0x8(%rsp), %rdx     ; 
lea 0xc(%rsp), %rcx     ; 
call sscanf

Clues that these leas are for locals:

  1. lea doesnt deref the registers. It did arithmathic operations on the address
     stored in the registers to calculate another address and passed it to rdx, rcx.
     rcx, rdx are pointers to the addresses %rsp+0x8 and %rsp+0xc

  2.  The compiler reserved 24 bytes for locals.  
      %rsp + 8 and %rsp + 12 are inside that reserved area.  

  3. The registers are passed to a function expecting pointers, like scanf/sscanf
</pre>
0x18 -> 24 bytes allocated.<br>
`x/6dw $rsp` shows all the values as decimals in the 24bytes alloated space


Probable func signature used here:-

`sscanf(input, "%d %d", &a, &b);`

`lea    0xc(%rsp),%rcx ` is the 4th arg (the &b)<br>
`lea    0x8(%rsp),%rdx ` is the 3rd arg (the &a)<br>
`mov    $0x4025cf,%esi ` is the "%d %d" string literal (2nd argument for sscanf)
<pre>
According to :

Argument #	Register (64-bit)
1	%rdi
2	%rsi
3	%rdx
4	%rcx
5	%r8
6	%r9
</pre>

As after sscanf returns: 
<pre>
(input given = 1 2 3)

(gdb) x/dw $rsp+0xc 
0x7fffffffdedc: 2 

(gdb) x/dw $rsp+0x8 
0x7fffffffded8: 1

(gdb) x/c 0x4025cf 
0x4025cf: 37 '%' 

(gdb) x/c 0x4025cf+0x1
0x4025cf: 37 'd'

(gdb) x/c 0x4025cf+0x2 
0x4025cf: 37 ' '

(gdb) x/c 0x4025cf+0x3
0x4025cf: 37 '%'

(gdb) x/c 0x4025cf+0x4 
0x4025cf: 37 'd'
</pre>


`phase_3 has 7 correct answers.`

The main instructions to look at are:-
<pre>
.
.
cmpl   $0x7,0x8(%rsp)
 ja    0x400fad <phase_3+106>
mov    0x8(%rsp),%eax 
jmp    *0x402470(,%rax,8)
.
.
</pre>

# Flags set by cmp:

`CF` looks at the operands in `unsigned` terms (whether the operands would require a borrow).

`OF`, `ZF`, and `SF` look at the `signed` result of the subtraction (whether the result is negative, zero, or whether an overflow occurred in signed terms).

# In phase_4:

`Found 3 correct answers`


