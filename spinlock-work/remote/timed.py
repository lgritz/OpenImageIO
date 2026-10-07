#!/usr/bin/env python3
"""Run a command; append its stdout+stderr to a log file, then print one line:
    wall=<sec> cpu=<sec> rc=<code>
CPU is the children's user+sys time (portable; no GNU time needed).
Usage: timed.py <logfile> <cmd> [args...]
"""
import os
import resource
import subprocess
import sys
import time

log = sys.argv[1]
cmd = sys.argv[2:]
r0 = resource.getrusage(resource.RUSAGE_CHILDREN)
t0 = time.monotonic()
with open(log, "ab") as f:
    f.write(("$ " + " ".join(cmd) + "\n").encode())
    f.flush()
    rc = subprocess.call(cmd, stdout=f, stderr=subprocess.STDOUT)
t1 = time.monotonic()
r1 = resource.getrusage(resource.RUSAGE_CHILDREN)
cpu = (r1.ru_utime - r0.ru_utime) + (r1.ru_stime - r0.ru_stime)
line = "wall=%.3f cpu=%.3f rc=%d" % (t1 - t0, cpu, rc)
with open(log, "a") as f:
    f.write(line + "\n")
print(line)
