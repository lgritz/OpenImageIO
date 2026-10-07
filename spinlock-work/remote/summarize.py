#!/usr/bin/env python3
"""Summarize a results/<RUN> directory as markdown (stdout).
Usage: summarize.py <results-dir>
"""
import glob
import os
import re
import sys
from collections import OrderedDict, defaultdict

D = sys.argv[1]


def read(fn):
    try:
        with open(os.path.join(D, fn), errors="replace") as f:
            return f.read()
    except OSError:
        return ""


def variants_order():
    here = os.path.dirname(os.path.abspath(__file__))
    out = []
    for line in open(os.path.join(here, "variants.txt")):
        f = line.split()
        if f and not f[0].startswith("#"):
            out.append(f[0])
    return out


VORDER = variants_order()


def vkey(v):
    return VORDER.index(v) if v in VORDER else 99


def table(rows, cols, cell, title, rowname="variant"):
    print(f"\n**{title}**\n")
    print(f"| {rowname} | " + " | ".join(str(c) for c in cols) + " |")
    print("|---" * (len(cols) + 1) + "|")
    for r in rows:
        print(f"| {r} | " + " | ".join(cell(r, c) for c in cols) + " |")


print(f"# Results: {os.path.basename(os.path.normpath(D))}\n")

# ---- machine ----
m = read("machine.txt")
if m:
    keep = [l for l in m.splitlines()
            if not re.match(r"\s*(Flags|Vulnerability|machdep.cpu.(features|leaf7|extfeatures))", l)]
    print("## Machine\n<details><summary>machine.txt</summary>\n\n```\n"
          + "\n".join(keep[:80]) + "\n```\n</details>")

pc = read("pausecost.txt")
if pc:
    print("\n## Pause instruction cost\n```\n" + pc.strip() + "\n```")

ct = read("ctest.txt")
if ct:
    lines = [l for l in ct.splitlines() if "tests passed" in l or "Failed" in l or "***" in l]
    print("\n## ctest (newest variant)\n```\n" + "\n".join(lines[-20:]) + "\n```")

# ---- testtex ----
# testtex threadtimes rows: "  16      2.29      0.4x   ..." ; first two fields are threads, seconds.
tt = defaultdict(dict)  # (tag) -> {variant: {threads: secs}}
itp = defaultdict(set)  # tag -> set of iters/thread values seen
for fn in glob.glob(os.path.join(D, "testtex-*.txt")):
    base = os.path.basename(fn)[len("testtex-"):-4]
    v, tag = base.split("-", 1)
    res = OrderedDict()
    for line in open(fn, errors="replace"):
        f = line.split()
        if len(f) >= 3 and f[0].isdigit() and re.match(r"^\d+\.\d+$", f[1]) and f[2].endswith("x"):
            res[int(f[0])] = float(f[1])
            mm = re.search(r"\((\d+) iters/thread\)", line)
            if mm:
                itp[tag].add(int(mm.group(1)))
    if res:
        tt[tag][v] = res
if tt:
    print("\n## testtex --threadtimes (seconds)")
    print("Weak scaling (fixed iters/thread): flat = perfect. "
          "Strong scaling (quick runs with --iters): halving = perfect.")
    for tag in sorted(tt):
        vs = sorted(tt[tag], key=vkey)
        cols = sorted({t for v in vs for t in tt[tag][v]})
        kind = "weak scaling" if len(itp[tag]) == 1 else "strong scaling"
        table(vs, cols, lambda r, c: ("%.2f" % tt[tag][r][c]) if c in tt[tag][r] else "",
              f"{tag} ({kind})")
        if "base" in tt[tag] and cols:
            tmax = cols[-1]
            b = tt[tag]["base"].get(tmax)
            if b:
                rel = ", ".join(f"{v} {b / tt[tag][v][tmax]:.2f}x" for v in vs
                                if v != "base" and tmax in tt[tag][v])
                print(f"\nSpeedup vs base at {tmax} threads: {rel}")

# ---- stats excerpts ----
for fn in sorted(glob.glob(os.path.join(D, "testtex-*stats*.txt"))):
    keep = []
    for line in open(fn, errors="replace"):
        if re.search(r"micro-cache|microcache|Tiles:|Peak|mutex|redundant|Find tile|find_tile|File I/O|Total pixel|Reads|wall=", line, re.I):
            keep.append(line.rstrip())
    if keep:
        print(f"\n<details><summary>{os.path.basename(fn)}</summary>\n\n```\n" + "\n".join(keep[:40]) + "\n```\n</details>")

# ---- spinlock / spinrw / parallel (variant threads wall= cpu= rc=) ----
def unit(step, title):
    s = read(f"{step}.txt")
    if not s:
        return
    data = defaultdict(dict)
    for line in s.splitlines():
        f = line.split()
        if len(f) < 4:
            continue
        v, n = f[0], int(f[1])
        kv = dict(x.split("=") for x in f[2:] if "=" in x)
        data[v][n] = kv
    # Program's own best-of-trials time, from the log.
    own = defaultdict(dict)
    for v in data:
        log = read(f"{step}-{v}.log")
        cur = None
        for line in log.splitlines():
            mm = re.search(r"--threads (\d+)", line)
            if line.startswith("$ ") and mm:
                cur = int(mm.group(1))
                continue
            f = line.split()
            if cur is not None and len(f) >= 2 and f[0] == str(cur):
                try:
                    own[v][cur] = float(f[1])
                except ValueError:
                    pass
    vs = sorted(data, key=vkey)
    cols = sorted({n for v in vs for n in data[v]})
    print(f"\n## {title}")
    table(vs, cols, lambda r, c: f"{own[r].get(c, '')}" if c in own[r] else "",
          "program-reported time (s, best of trials)")
    table(vs, cols, lambda r, c: (f"{data[r][c]['wall']}/{data[r][c]['cpu']}" if c in data[r] else ""),
          "whole process wall/cpu seconds (all trials)")


unit("spinlock", "spinlock_test")

# parallel_test reports "launch N threads/sec"
par = defaultdict(dict)
for fn in glob.glob(os.path.join(D, "parallel-*.log")):
    v = os.path.basename(fn)[len("parallel-"):-4]
    cur = None
    for line in open(fn, errors="replace"):
        mm = re.search(r"--threads (\d+)", line)
        if line.startswith("$ ") and mm:
            cur = int(mm.group(1))
        mm = re.search(r"launch ([\d.]+) threads/sec", line)
        if mm and cur is not None and cur not in par[v]:
            par[v][cur] = float(mm.group(1))
if par:
    vs = sorted(par, key=vkey)
    cols = sorted({n for v in vs for n in par[v]})
    print("\n## parallel_test")
    table(vs, cols, lambda r, c: ("%.3g" % par[r][c]) if c in par[r] else "",
          "parallel_for task launches per second (higher is better)")

# spin_rw_test --wedge rows: "16\t1.2s\t  1.2s, range 0.0\t(N iters/thread)"
rw = defaultdict(dict)
for fn in glob.glob(os.path.join(D, "spinrw-*.log")):
    v = os.path.basename(fn)[len("spinrw-"):-4]
    for line in open(fn, errors="replace"):
        mm = re.match(r"\s*(\d+)\t.*?([\d.]+)s, range", line)
        if mm:
            rw[v][int(mm.group(1))] = float(mm.group(2))
if rw:
    vs = sorted(rw, key=vkey)
    cols = sorted({n for v in vs for n in rw[v]})
    print("\n## spin_rw_test (strong scaling; seconds, best of 3)")
    table(vs, cols, lambda r, c: ("%.1f" % rw[r][c]) if c in rw[r] else "", "spin_rw_test --wedge")

# ---- spinbench ----
for fn, title in (("spinbench.txt", "spinbench (all nodes)"),
                  ("spinbench-node0.txt", "spinbench (NUMA node 0 only)")):
    s = read(fn)
    if not s:
        continue
    print(f"\n## {title}: wall/cpu ns per op")
    unc = [l.split() for l in s.splitlines() if " unc " in l]
    if unc:
        print("\nUncontended ns: " + ", ".join(f"{f[2]}={f[3]}" for f in unc if len(f) > 3 and f[0].endswith("17o3")))
    d = defaultdict(dict)
    order = []
    for line in s.splitlines():
        f = line.split()
        if len(f) < 6 or f[1] != "con":
            continue
        key = (f[0], f[4], f[2])
        if key not in order:
            order.append(key)
        d[key][int(f[3])] = f[5]
    for b in sorted({k[0] for k in order}):
        for shape in ("tiny", "medium", "coloc"):
            keys = [k for k in order if k[0] == b and k[1] == shape]
            if not keys:
                continue
            cols = sorted({t for k in keys for t in d[k]})
            table([k[2] for k in keys], cols,
                  lambda r, c: d[(b, shape, r)].get(c, ""), f"{b} {shape}", "lock")

# ---- perf ----
for fn in sorted(glob.glob(os.path.join(D, "c2c-*.txt")) + glob.glob(os.path.join(D, "perf-*-w*.txt"))):
    print(f"\n(perf output: {os.path.basename(fn)})")

print("\n## Progress log tail\n```\n" + "\n".join(read("progress.log").splitlines()[-15:]) + "\n```")
