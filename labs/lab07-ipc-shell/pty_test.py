#!/usr/bin/env python3
"""Запускає програму в pseudo-terminal і «друкує» в неї команди, як людина
в терміналі (включно з Ctrl+C = байт 0x03). Використовується run.sh для
тестів, яким потрібен справжній термінал: Ctrl+C у mini_shell і signal_int.

Використання: pty_test.py PROGRAM  STEP...
  STEP: "text\n" — надіслати текст;  "^C" — Ctrl+C;  "sleep:N" — пауза N с.
"""
import os, pty, sys, time, select

def drain(fd, out, timeout=0.3):
    end = time.time() + timeout
    while time.time() < end:
        r, _, _ = select.select([fd], [], [], 0.05)
        if r:
            try:
                data = os.read(fd, 4096)
            except OSError:
                return False
            if not data:
                return False
            out.append(data)
    return True

prog, steps = sys.argv[1], sys.argv[2:]
pid, fd = pty.fork()
if pid == 0:
    os.execvp(prog, [prog])

out = []
drain(fd, out, 0.5)
for step in steps:
    if step == "^C":
        os.write(fd, b"\x03")
    elif step.startswith("sleep:"):
        drain(fd, out, float(step[6:]))
        continue
    else:
        os.write(fd, step.encode().decode("unicode_escape").encode())
    drain(fd, out, 0.6)

drain(fd, out, 1.0)
_, status = os.waitpid(pid, os.WNOHANG) if True else (0, 0)
sys.stdout.write(b"".join(out).decode(errors="replace").replace("\r", ""))
if status == 0 and _ == 0:
    print("\n[pty_test] program still running -> sending SIGKILL")
    os.kill(pid, 9); os.waitpid(pid, 0)
elif os.WIFSIGNALED(status):
    print(f"\n[pty_test] program killed by signal {os.WTERMSIG(status)}")
else:
    print(f"\n[pty_test] program exited with status {os.WEXITSTATUS(status)}")
