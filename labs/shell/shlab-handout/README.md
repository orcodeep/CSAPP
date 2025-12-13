# eval() and job control

The main steps for handling a non-built-in command in eval() are:

- Parse the command line → get argv[] and background flag (bg)

- Fork a child process

In the child:

- Set a new process group (setpgid(0, 0)) so it doesn’t receive signals from the shell

- Call execve() to run the program

In the parent (shell):

- Add the child to the job list with its PID, state (FG or BG), and command line<br><br>
`ST` (stopped) is set later in your signal handler if the job is stopped.<br>
AND<br>
`UNDEF` is for empty slots — not relevant when adding a new job.

- If foreground → wait for it (waitfg())

- If background → print job info and return immediately




