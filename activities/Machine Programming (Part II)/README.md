test op sets:   ZF, SF, (& some other not required for this lab)

test clears: CF = 0, OF = 0

call cmovc(32767, 1) = 0 not 1<br>
Because:-<br>
cmovc:

    mov    $0x0,%eax
    test   %di,%di
    cmovb  %esi,%eax
    ret

%edi = 32767 = 0x00007FFF<br>
%di = 0x7FFF,<br>
test computes 0x7FFF & 0x7FFF = 0x7FFF<br>
Then sets flags based on that result.<br>
Flags:

    ZF = 0
    (result is nonzero)

    SF = 0
    The sign bit of a 16-bit value is bit 15.(starting from bit 0 in rightmost)
    0x7FFF = 0111 1111 1111 1111 → sign bit = 0 → not negative.

    CF = 0
    (test always clears carry)

    OF = 0
    (test always clears overflow)

test clears the CF flag<br>
so %esi = 1 is not moved into %eax before returning .
