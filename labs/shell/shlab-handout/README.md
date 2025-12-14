# eval() and job control

The main steps for handling a non-built-in command in eval() are:

- Parse the command line → get argv[] and background flag (bg)

- Fork a child process

In the child:

- Set a new process group (`setpgid(0, 0)`) so it doesn’t receive signals from the shell

- Call execve() to run the program

In the parent (shell):

- Add the child to the job list with its PID, state (`FG` or `BG`), and command line<br><br>
`ST (stopped) is set later in your signal handler if the job is stopped`.<br>
AND<br>
`UNDEF is for empty slots — not relevant when adding a new job`.

- If foreground → wait for it (`waitfg()`)

- If background → print job info and return immediately

# do_bgfg()

## The `fg` command can do both:

1\. Bring a stopped job to the foreground

- Job state is `ST` (stopped)

- `fg` changes state → `FG` and sends `SIGCONT`

- Each job has its own process group, set with setpgid() in eval().

- **A job can have multiple processes (e.g., a pipeline), and you want them all to resume.**

- When you send kill(-pid, SIGCONT), the negative PID sends the signal to all processes in that job’s process group.

2\. Bring a running background job to the foreground

- Job state is `BG`

- `fg` changes state → `FG`

Waits for it to finish before the prompt is printed agin.

## The bg command:

The bg <job> command restarts <job> by sending it a SIGCONT signal, and then runs it in

Unlike fg it doesnt wait for the job to finish before printing the prompt again.

# waitfg()

**“waitfg waits for a foreground job to complete.”**

Here, “**complete**” usually refers to the job no longer being in the foreground.

In practice, a foreground job is “done” in terms of waitfg if:

- It terminates → all processes in the job exit.

- It is stopped → the user pressed Ctrl+Z; the job is no longer running in the foreground.

So “**complete**” does not necessarily mean terminated — it means the shell can return control to the user because the foreground job is no longer actively running.


`SIGCHLD` tells the shell that one of its children changed state (`terminated`, `stopped`, `continued`).

The shell does not receive the SIGTSTP/SIGCONT directly — those go to the child process.

1\. Using `sigsuspend`

- sigsuspend temporarily replaces the signal mask and suspends the shell until a signal arrives.

- In job control, the signal we care about is SIGCHLD (child changed state).

- When the signal arrives, sigsuspend returns, allowing the shell to call waitpid to learn what happened.

2\. Using waitpid

- After waking from sigsuspend, the shell calls waitpid(-pgid, &status, WNOHANG | WUNTRACED).

- `WNOHANG` ensures waitpid doesn’t block — we already slept in sigsuspend.

- `WUNTRACED` ensures we detect stopped children.


`waitpid` both reports the status and actually reaps the child:-

Two roles of waitpid:

1\. Reaping the child

- When a child terminates, it becomes a zombie: the kernel keeps its exit status and process table entry around.<br><br>
`waitpid` only reaps a child process if it has terminated (`WIFEXITED` or `WIFSIGNALED`).

If the child (or group leader) is stopped (`WIFSTOPPED`), **it has not terminated yet, _so it is not reaped_.**

- waitpid(pid, &status, options) tells the kernel:<br><br>“I’m the parent, I’m done with this child, you can remove its process table entry now.”<br><br>That’s what reaping means — removing the zombie and freeing kernel resources.

2\. Returning status information. 

So even though the child is gone, the exit info is preserved in status, allowing the shell to update the job list or print a message.

- The status argument tells the parent why the child changed state: exited normally, killed by a signal, stopped, or continued.

- You inspect it using macros: WIFEXITED, WIFSIGNALED, WIFSTOPPED, etc.

**In a job, some processes might stop temporarily, but the job is considered running until the grp leader stops or terminates**.

**Only the leader’s stop or termination triggers shell-level job state changes (ST or removal).**





