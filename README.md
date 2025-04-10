# Mini Shell in C

This project is a simple shell program written in C that simulates basic features of a Unix shell. It was developed as part of the Operating Systems course at the Hebrew University.

The shell supports executing programs, background processes, single piping, and input/output redirection, using low-level system calls like `fork`, `execvp`, `pipe`, `dup2`, and `sigaction`. It handles signals properly and prevents zombie processes.

## Features

- Run commands like `ls`, `sleep`, `cat`, etc.
- Background execution using `&` (e.g., `sleep 5 &`)
- Input redirection using `<` (e.g., `cat < file.txt`)
- Output redirection using `>` (e.g., `ls > out.txt`)
- Single pipe between two commands (e.g., `cat file.txt | grep hello`)
- Signal handling with `SIGINT` and `SIGCHLD`
- Prevents zombie processes using `waitpid`

## How It Works

The shell runs in an infinite loop:
1. Reads user input and splits it into arguments.
2. Detects if the command uses `|`, `<`, `>`, or `&`.
3. Based on the symbol, it forks child processes and uses the appropriate system calls to redirect input/output or create a pipe.
4. The parent shell waits for child processes if necessary, or continues immediately for background commands.
5. The shell handles interrupts (Ctrl+C) so that the shell itself doesn’t exit, but foreground child processes do.

## Compilation

To compile the project, run:

gcc -O3 -D_POSIX_C_SOURCE=200809 -Wall -std=c11 shell.c myshell.c
