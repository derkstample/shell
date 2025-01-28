# Simple shell

A **C implementation** of a mini Linux shell, inspired by `bash`. `Makefile` included for convenience.

---

## Features
- prints cwd for each new command
- parses input as tokens, executing tokens using forked threads and execvp
- cd
- `<` and `>` operators (IO redirection)
- history with 100 (configurable) entries
- `!` operator (history recall)
- `&` operator (background execution)
- automatic reaping of zombie processes.

---

## History
- history is stored in `.shell_history` (like `bash`)
- file is created in the current directory of `./shell` if it does not exist
- 100 entries are saved, after which history stops updating (although it would be trivial to modify the buffer to be circular/delete older entries).

---

## Tests
Run `make tests` to create testing programs for the shell.
Tests included:
- `waitTest` and `smallerWaitTest`: simply wait 10 and 5 seconds (respectively), then print a line to cout, useful for verifying background execution works as expected.
- `redirTest`: read a line and print it back out, useful for verifying IO redirection works as expected.