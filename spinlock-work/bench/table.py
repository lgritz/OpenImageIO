#!/usr/bin/env python3
# Format run.bash output as tables: rows=variant, cols=threads, cell=wall/cpu.
import sys
from collections import defaultdict

fn = sys.argv[1]
which = sys.argv[2] if len(sys.argv) > 2 else "spinbench20"
d = defaultdict(dict)
threads = set()
order = []
for line in open(fn):
    f = line.split()
    if len(f) < 4 or f[0] != which or f[1] != "con":
        continue
    _, _, v, nt, shape, *r = f
    key = (shape, v)
    if key not in order:
        order.append(key)
    d[key][int(nt)] = r[0] if r else "-"
    threads.add(int(nt))
threads = sorted(threads)
for shape in ("tiny", "medium", "coloc"):
    print(f"\n{which} -- {shape} critical section, wall/cpu ns per op")
    print(f"{'variant':24}" + "".join(f"{t:>13}" for t in threads))
    for key in order:
        if key[0] != shape:
            continue
        print(f"{key[1]:24}" + "".join(f"{d[key].get(t, ''):>13}" for t in threads))
