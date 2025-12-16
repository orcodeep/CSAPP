# Note

1\. For **test03** to pass- sigchld_handler must be made because only that changes that state of the terminated processes. And `waitfg()` uses `fgpid()` which depends on the state of the processes to remove a foreground job. 

If we do not implement sigchld_handler then shell will keep waiting infinitely after executing a foreground process and not print the prompt.

2\. For **test06** to pass- we need to make `sigint_handler` and the `sigchld_handler` should also print the details of how and by which signal the job got killed. 

The rest will pass after finishing `do_bgfg()` and `sigstp_handler`.

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

`bg`, `fg` usage:- `bg %Jid`, `fg %Jid`

**A job can have multiple processes and you want them all to resume.**

Each job has its own process group, set with setpgid() in eval().

When you send kill(-pid, SIGCONT), the negative PID sends the signal to all processes in that job’s process group.

## The `fg` command can do both:

1\. Bring a stopped job to the foreground

If Job state is `ST` (stopped)

- `fg` changes state → `FG` and sends `SIGCONT`

2\. Bring a running background job to the foreground

If Job state is `BG`

- `fg` changes state → `FG`

**Waits for it to finish before the prompt is printed agin. It doesnt print job info.**

## The bg command:

<ul>
<li>The bg %&lt;jid&gt; command restarts job by sending it a `SIGCONT` signal, and then runs it in background.</li>

<li>If user gives `bg <pid>`- pid must be the pid of the grp leader of a job i.e the pgid of the job. Its stored in the job struct as `pid`.</li>
</ul>


**Unlike fg it doesnt wait for the job to finish before printing the prompt again. But it prints the job info before printing the prompt again.**


# waitfg()

**“waitfg waits for a foreground job to complete.”**

sigsuspend temporarily replaces the signal mask and suspends the shell until a signal arrives.

Foreground wait = logical wait, not `waitpid()`

The processes of the fg job will be reaped in the sigchild handler not by the shell while its waiting for it to finish.

## why waking from sigsuspend on only `SIGCHILD` is wrong

`SIGINT` / `SIGSTP` would be delayed

we shouldnt ignore `SIGINT`/`SIGSTP` because otherwise the signal_handlers for them wouldn't be able to run.

shell would feel unresponsive.


# Using waitpid

The shell does not receive the SIGTSTP/SIGCONT directly — those go to the child process. waitpid directly asks the kernel for state change.

Return value > 0 (a PID) means- one child matching pid has changed state. That child may be:

- terminated normally → WIFEXITED(status)
- killed by a signal → WIFSIGNALED(status)
- stopped → WIFSTOPPED(status)

Return value == 0 means- if `WNOHANG` was specified, child/children exists but no state change yet.

- so the `sigchld_handler` ran but it didnt get any processes to reap and also none were stopped.

Return value == -1 means- Error (or no children exist: `errno` = `ECHILD`)

**In a job, some processes might stop temporarily, but the job is considered running until the grp leader stops or terminates**.

**Only the leader’s termination or stopping triggers shell-level job state changes (deletion or ST).**


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











