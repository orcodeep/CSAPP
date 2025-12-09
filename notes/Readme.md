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

# Hex and raw:

what objdump shows(`48` `89` `e5` `90` `90`) is ascii text This is NOT what the CPU executes. This is just human-readable text representing bytes. 

Raw = actual binary bytes in memory (`0x48` `0x89` `0xE5` `0x90` `0x90`)<br>
Thats why when printed many show up as `���` because they dont have valid 
ascii representation for them.

# RIP and execution:

`rip` normally always points into the `.text` section

During normal (non‑overflow) execution:

- RIP points to an address in `.text`

- CPU fetches instructions from `.text`

- Executes them one-by-one (unless a jump/call/branch changes control flow 
  and takes it some other address in `.text`) 

ret is just another instruction in the .text section. 
So `rip` reaches this instruction just like any other.

what does ret do:

- ret pops a value from the stack

- That popped value must be an address in .text (normally). 
  `so ret pop a stack address and that stack address contains an address inside the .text section`

- `rip` becomes that value

- CPU continues executing in `.text`

# x86-64 instructions are self decoding

The CPU starts at the current instruction pointer(rip), reads a byte and interprets it as an opcode.

Based on the op code, the CPU knows:

- whether there are prefixes

- whether there are ModR/M byte

- whether it has SIB byte

- whether it has a displacement

- whether it has an immediate value 

- total length of the instruction, etc..

So the CPU knows exactly how many bytes belong to that instruction.

Then it moves the rip forward by that number of instructions.

So once the rip enters the buffer it knows how to execute each instruction.

# Child process termination

When a child process terminates, it becomes a zombie unitl its parent calls `wait()` (or `waitpid()`) to collect its exit status.

If a parent doesnt call `wait()`, the child remains a zombie.

## Reaping by init

If the parent process itself terminates before collecting the child's exit status, the child is adopted by `init`(PID 1) or the system's equivalent (like systemd).

`init` automatically calls `wait()` on orphaned children, so the child is reaped and removed from the process table.

### What `wait()` returns

On success:

- Returns process ID (PID) of hre terminated child that was reaped.<br><br>
<pre>
pid_t pid = wait(NULL);
</pre>

On failure:

- Returns -1 and sets `errno` to indicate the error.

- Common errors:

    - `ECHILD`: No child process exist 

    - `EINTR`: Interrupted by a signal before any child terminated




