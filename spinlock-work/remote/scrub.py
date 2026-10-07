#!/usr/bin/env python3
"""Replace production texture paths (and their basenames) with prodtexN in
result files, so the results can be committed without leaking file names.
Usage: scrub.py <listfile> <resultfile>...
"""
import os
import sys

names = [l.strip() for l in open(sys.argv[1]) if l.strip()]
subs = []
for i, p in enumerate(names):
    subs.append((p, "prodtex%d" % i))
    subs.append((os.path.abspath(p), "prodtex%d" % i))
    subs.append((os.path.basename(p), "prodtex%d" % i))
# Longest first, so full paths are replaced before basenames.
subs.sort(key=lambda s: -len(s[0]))
for fn in sys.argv[2:]:
    with open(fn, errors="replace") as f:
        s = f.read()
    for a, b in subs:
        if a:
            s = s.replace(a, b)
    with open(fn, "w") as f:
        f.write(s)
