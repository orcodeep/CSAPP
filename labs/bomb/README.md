There should be a `newline` character after each answer in the inputs file 
and each ans should not have any trailing spaces.

# In phase_1:

The answer can be found even without using gdb, just by using `strings` 
command and intuition.

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

The input string is parsed by sscanf inside `<read_six_numbers>` 

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

# In phase_5:

phase_5 has no call to sscanf and input string already i %rdi:
<pre>
Phase_4 parses the input string using sscanf, 
extracting numbers according to the format string (like "%d %d").  
Those numbers are then stored in local variables on the stack, 
which the phase logic uses.

Phase_5: Like phase_1 It doesn’t parse the input at all. 
It just takes the string pointer already read by read_line() 
in main() and manipulates the characters directly.

(input given = Hello)

(gdb) x/s $rdi 
0x6038c0 <input_strings+320>:   "Hello"
</pre>
%rdi contains the address of the string "Hello", 

    (gdb) x/x $rdi  
    0x6038c0 <input_strings+320>:   0x48 (which is H)

    (gdb) x/x $rdi+0x1  
    0x6038c1 <input_strings+321>:   0x65 (which is e)

    (gdb) x/x $rdi+0x2  
    0x6038c2 <input_strings+322>:   0x6c (which is l)

    (gdb) x/x $rdi+0x3  
    0x6038c3 <input_strings+323>:   0x6c 

    (gdb) x/x $rdi+0x4  
    0x6038c4 <input_strings+324>:   0x6f (which is o)


Then `cmpb $0x0,(%rdi)`:

- Dereferences %rdi → reads the first byte at that address

- The first byte of "Hello" is 'H'<br>
  cmp reads 8bits of that word which is still:-<br>
  1001000 (ASCII of 'H' = 0x48 = 72 decimal.)

- Reads the value at %rdi → 72 ('H')

- Compares it to 0 → obviously not equal

- Zero flag (ZF) = 0

`<string_length>`:

- It checks for the whole string if '\0' or 0x0 is reached 
  when %rdx contains "" i.e an empty string

  (In C/assembly, an empty string contains exactly one byte<br>
   0x00 ← the null terminator) it returns
  and it put the strlength in %eax 


`movzbl (%rbx,%rax,1),%ecx` Reads 1 byte from `memory[rbx+rax*1]`,
`zero-extends` it to 32 bits, and stores it in %ecx

`cl` is the lowest (least significant) byte of the 32-bit register ecx

`phase_5 can have many answers`

Main instructions to look at:

    1. Understanding what strings_not_equal is doing,  
       what it returns in eax especially what are its arguments.
    
    2. <phase_5+41> to <phase_5+52> it is doing some calculation  
       which is based on the input given. 
    
    3. <phase_5+55>, <phase_5+62>


`When ans to phase_5 is put in the inputs file it should be made sure that there are no trailing spaces`

# In phase_6:

To see where `<read_six_numbers>` read(`parsed`) the input:<br>

    x/80wd $rsp (0x50 = D 80)

It will be found starting from `$rsp`  

There is a lot of looping which checks:

- If the numbers after an element in the input array are different
  than the element. This will be checked for each element.

- It will also be checked that the values are <= 6.

- Then another array of pointers gets created using the 
  input values.

- The instructions `cmp %eax,(%rbx)` and `jge 0x4011ee <phase_6+250>`
  on `<phase_6+241>`and the next line make the real comparisions that 
  decide if the input is correct or not. 

- There are a total of 5 comparisions made which decide if given input
  is correct or not. `mov $0x5,%ebp` on `<phase_6+230>` and `sub $0x1,%ebp`, `jne 0x4011df <phase_6+235>`on `<phase_6+254>` and the next line make sure that only 5 comparisions are made. 

- Like all arithmetic instructions, `sub` sets the Zero Flag `ZF` based 
  on whether the result is zero.

  If %ebp - 1 == 0, then ZF = 1 → `jne` does not jump  to `<phase_6+235> ` 
  → loop ends. and `add $0x50,%rsp` is reached. 

To double deref a pointer to a pointer:<br>
`x/4wd *(long*)($rsp+32)`