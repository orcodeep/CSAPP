In phase_2:
<pre>    
0x0000000000400f25 <+41>:    add    $0x4,%rbx                                            0x0000000000400f29 <+45>:    cmp    %rbp,%rbx                                            0x0000000000400f2c <+48>:    jne    0x400f17 <phase_2+27>                                0x0000000000400f2e <+50>:    jmp    0x400f3c <phase_2+64> 
</pre>
This makes sure all the elements of the array are checked before jumping to phase_2 + 64
<pre>
%rbp has address of the end of array and  

%rbx has address of the curretn element of array

cmp %rbp,%rbx sets ZF=1 if addr[rbx]==addr[rbp]
when ZF=1, jne=false => jmp will be excuted
</pre>

Note:

The `stack` grows `downward`

So when we say increment the stack we mean<br>
`shrink` the stack<br>
`add $0xXX, %rsp` instruction shrinks the stack

when we say decrement the stack we mean<br>
`grow` the stack<br>
`sub $0xXX,%rsp` grows the stack

