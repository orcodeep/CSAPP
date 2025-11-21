# Callee and Caller saved Registers

## Caller-Saved Registers

The caller saved registers are registers that normally always get used by a function call

These are registers that a function (callee) is free to overwrite during its execution.

They are typically used for:

- Passing arguments (`rdi`, `rsi`, `rdx`, `rcx`, `r8`, `r9`)

- Returning values (`rax`)

- Temporary computations inside the function (`r10`, `r11`)

Because the callee can use them without saving, the caller is responsible if it wants to preserve any values in these registers across the call.

## Callee-Saved Registers

These are registers that the caller expects to remain unchanged across a function call.

By default, a function (callee) does not need to use them.

If the callee does want to use one of these registers, it must save its original value (usually on the stack) and restore it before returning.

Purpose: `Allow the caller to store long-lived values in certain registers across a function call without worrying that the callee will overwrite them.`

Rule: If the callee wants to use a callee-saved register (like rbx, rbp, r12–r15):

- It must save the original value (usually by pushing it on the stack).

- It can then use the register freely.

- Before returning, it must restore the original value so the caller sees the register unchanged.